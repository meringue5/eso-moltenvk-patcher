"""Non-game native/Python differential fixtures; no installed ESO is required."""
from __future__ import annotations
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest

from analyze_vk_calls import image_base, load_sections
from eso_update import audit_manifest, locate_inactive_pacing, profile_manifest
from generate_targets import macho_uuid
import uuid

ROOT = Path(__file__).resolve().parent.parent
NAMES = (
    'vkGetPhysicalDeviceProperties', 'vkGetPhysicalDeviceMemoryProperties',
    'vkGetInstanceProcAddr', 'vkAllocateMemory', 'vkFreeMemory', 'vkMapMemory',
    'vkUnmapMemory', 'vkBindBufferMemory', 'vkBindImageMemory',
    'vkGetBufferMemoryRequirements', 'vkGetImageMemoryRequirements',
    'vkCreateBuffer', 'vkDestroyBuffer', 'vkCreateImage', 'vkDestroyImage',
    'vkEndCommandBuffer', 'vkCmdEndRenderPass',
)


@unittest.skipUnless(sys.platform == 'darwin' and shutil.which('xcrun'), 'requires macOS toolchain')
class NativeProfiles(unittest.TestCase):
    @classmethod
    def run_command(cls, args, **kwargs):
        return subprocess.run([str(a) for a in args], capture_output=True, text=True, check=True, **kwargs)

    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix='eso-native-profile-', dir='/private/tmp')
        cls.work = Path(cls.temp.name)
        obj_source = cls.work/'runtime.s'
        obj_source.write_text('.text\n' + '\n'.join(
            f'.globl _{name}\n_{name}:\n.byte 0x55,0x48,0x89,0xe5,0xb8,{i+1},0,0,0,0x5d,0xc3\n.space 5,0x90'
            for i, name in enumerate(NAMES)) + '\n.subsections_via_symbols\n')
        cls.obj = cls.work/'MoltenVK-x86_64-master.o'
        cls.run_command(['xcrun', 'clang', '-arch', 'x86_64', '-c', obj_source, '-o', cls.obj])
        cls.archive = cls.work/'MoltenVK'
        cls.run_command(['xcrun', 'ar', '-rcs', cls.archive, cls.obj])
        cls.runtime = cls.work/'runtime.dylib'
        cls.run_command(['xcrun', 'clang', '-arch', 'x86_64', '-dynamiclib', cls.obj, '-o', cls.runtime])
        cls.a = cls.make_executable('a', 0, 0, False)
        cls.b = cls.make_executable('b', 32, 96, True)
        symbols = cls.symbols(cls.a)
        object_symbols = cls.symbols(cls.obj)
        base = image_base(load_sections(cls.a))
        data = cls.a.read_bytes()
        manifest = {
            'schema_version': 2, 'sha256': hashlib.sha256(data).hexdigest(),
            'uuid': str(uuid.UUID(bytes=macho_uuid(data))),
            'original_bink_sha256': '0'*64, 'retagged_original_bink_sha256': '1'*64,
            'description': 'Non-game synthetic fixture',
            'proc_addr_slots': {'vkGetInstanceProcAddr': hex(symbols['_gipa']-base),
                                'vkGetDeviceProcAddr': hex(symbols['_gdpa']-base)},
            'targets': [{'symbol': name, 'image_offset': hex(symbols['_'+name]-base),
                         'expected_bytes': data[symbols['_'+name]-base:symbols['_'+name]-base+12].hex()} for name in NAMES],
            'experimental_targets': {'inactive_pacing': locate_inactive_pacing(cls.a)},
        }
        delta = symbols['_'+NAMES[0]]-base-object_symbols['_'+NAMES[0]]
        cls.manifest = profile_manifest(cls.a, cls.archive, manifest, cls.obj.name, delta, cls.runtime)
        manifest_path = cls.work/'reference.json'
        manifest_path.write_text(json.dumps(cls.manifest))
        cls.run_command([sys.executable, ROOT/'tools/generate_update_rules.py', '--manifest', manifest_path,
                         '--exe', cls.a, '--archive', cls.archive, '--output', cls.work/'generated_update_rules.h'])
        cls.helper = cls.work/'audit'
        cls.run_command(['xcrun', 'clang', '-Wall', '-Wextra', '-Werror', '-O1',
                         '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                         '-I', ROOT/'src', '-I', cls.work, ROOT/'tools/compat_audit.c',
                         ROOT/'src/update_profile.c', ROOT/'src/third_party/hde64/hde64.c', '-o', cls.helper])

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    @classmethod
    def symbols(cls, path):
        output = cls.run_command(['nm', '-arch', 'x86_64', '-n', path]).stdout
        result = {}
        for line in output.splitlines():
            fields = line.split()
            if len(fields) == 3 and fields[1].lower() in ('t', 'd', 's'):
                result[fields[2]] = int(fields[0], 16)
        return result

    @classmethod
    def make_executable(cls, label, host_pad, object_pad, chained):
        root = cls.work/label
        mac = root/'game_mac/pubplayerclient/eso.app/Contents/MacOS'
        mac.mkdir(parents=True)
        stamp = root/'depot/_databuild/databuild.stamp'
        stamp.parent.mkdir(parents=True)
        stamp.write_text('fixture.live.123\n2026/10/24\n1.2.3\n')
        source = root/'host.s'
        source.write_text(f'''.text
.globl _main
_main:
.space {host_pad},0x90
leaq _active(%rip),%r13
cmpb $0,(%r13)
jne L_test
movl $100000,%edi
callq _usleep
jmp L_reload
.space 24,0x90
L_reload:
movb 0x18(%rbx),%al
L_test:
testb %al,%al
''' + '\n'.join(f'callq _{name}' for name in NAMES) + '''
xorl %eax,%eax
retq
.globl _queries
_queries:
leaq L_instance(%rip),%rsi
callq *_gipa(%rip)
leaq L_buffer(%rip),%rsi
callq *_gdpa(%rip)
retq
''' + f'.space {object_pad},0x90\n' + '''
.section __TEXT,__cstring,cstring_literals
L_instance: .asciz "vkCreateInstance"
L_buffer: .asciz "vkCreateBuffer"
.data
.globl _gipa,_gdpa,_active
_gipa: .quad 0
_gdpa: .quad 0
_active: .byte 0
.p2align 3
_owned_reference: .quad _vkGetPhysicalDeviceProperties
.subsections_via_symbols
''')
        exe = mac/'eso'
        cls.run_command(['xcrun', 'clang', '-arch', 'x86_64', '-mmacosx-version-min=12.0',
                         '-Wl,-fixup_chains' if chained else '-Wl,-no_fixup_chains', source, cls.obj, '-o', exe])
        return exe

    def native(self, *args):
        return subprocess.run([str(self.helper), *map(str, args)], capture_output=True, timeout=10)

    def derive(self, exe):
        result = self.native('--derive', exe, self.archive)
        self.assertEqual(result.returncode, 0, result.stderr.decode())
        return result.stdout

    def test_independent_relocations_match_python_auditor(self):
        candidate, failures = audit_manifest(self.b, self.archive, self.manifest, self.runtime, 'fixture update')
        self.assertEqual(failures, [])
        fields = dict(line.split(b'=', 1) for line in self.derive(self.b).splitlines() if not line.startswith(b'target='))
        self.assertEqual(int(fields[b'placement'], 16), int(candidate['analysis']['legacy_moltenvk']['link_delta'], 0))
        pacing = fields[b'pacing'].decode().split(':')
        self.assertEqual(int(pacing[0], 16), int(candidate['experimental_targets']['inactive_pacing']['image_offset'], 0))
        targets = [line.decode().split(':') for line in self.derive(self.b).splitlines() if line.startswith(b'target=')]
        self.assertEqual([int(row[2], 16) for row in targets], [int(t['image_offset'], 0) for t in candidate['targets']])
        old = self.manifest['experimental_targets']['inactive_pacing']
        self.assertNotEqual(int(candidate['targets'][0]['image_offset'], 0)-int(self.manifest['targets'][0]['image_offset'], 0),
                            int(pacing[0], 16)-int(old['image_offset'], 0))

    def test_profiles_are_canonical_and_bound_to_executable(self):
        profile = self.work/'valid.profile'
        raw = self.derive(self.a)
        profile.write_bytes(raw);profile.chmod(0o600)
        result = self.native('--verify', self.a, self.archive, profile)
        self.assertEqual(result.returncode, 0, result.stderr.decode())
        self.assertEqual(result.stdout.strip().decode(), hashlib.sha256(raw).hexdigest())
        self.assertEqual(self.native('--verify', self.b, self.archive, profile).returncode, 3)
        for bad in (raw+b'extra=1\n', raw.replace(b'schema=1', b'schema=2'), raw[:-1],
                    raw.replace(b'target=0:', b'target=1:'), b'x'*4097, b''):
            profile.write_bytes(bad)
            self.assertEqual(self.native('--verify', self.a, self.archive, profile).returncode, 3)
        profile.write_bytes(raw);profile.chmod(0o644)
        self.assertEqual(self.native('--verify', self.a, self.archive, profile).returncode, 3)
        link = self.work/'link.profile';link.symlink_to(profile)
        self.assertEqual(self.native('--verify', self.a, self.archive, link).returncode, 3)

    def test_changed_patch_pacing_and_header_are_refused(self):
        original = self.b.read_bytes()
        candidate, failures = audit_manifest(self.b, self.archive, self.manifest, self.runtime, 'fixture')
        self.assertFalse(failures)
        offsets = [int(candidate['targets'][0]['image_offset'], 0),
                   int(candidate['experimental_targets']['inactive_pacing']['image_offset'], 0), 0]
        try:
            for offset in offsets:
                bad = bytearray(original);bad[offset] ^= 1;self.b.write_bytes(bad)
                self.assertEqual(self.native('--derive', self.b, self.archive).returncode, 3)
        finally:
            self.b.write_bytes(original)

    def test_changed_archive_and_build_stamp_are_refused(self):
        archive = self.work/'bad.archive';archive.write_bytes(self.archive.read_bytes()+b'x')
        self.assertEqual(self.native('--derive', self.a, archive).returncode, 3)
        stamp = self.work/'b/depot/_databuild/databuild.stamp'
        original = stamp.read_bytes()
        try:
            stamp.write_text('invalid\n')
            self.assertEqual(self.native('--derive', self.b, self.archive).returncode, 3)
        finally:
            stamp.write_bytes(original)

    def test_malformed_macho_metadata_is_refused_without_memory_errors(self):
        original = self.b.read_bytes()
        changes = [(16, 0xffffffff), (32+4, 0), (20, 0xffffffff)]
        cursor = 32
        count = struct.unpack_from('<I', original, 16)[0]
        for _ in range(count):
            command, size = struct.unpack_from('<II', original, cursor)
            if command in (0x26, 0x80000034):  # function starts / chained fixups
                changes.append((cursor+8, 0xffffffff))
                changes.append((cursor+12, 0xffffffff))
            cursor += size
        try:
            for offset, value in changes:
                bad = bytearray(original)
                struct.pack_into('<I', bad, offset, value)
                self.b.write_bytes(bad)
                result = self.native('--derive', self.b, self.archive)
                self.assertEqual(result.returncode, 3, result.stderr.decode())
        finally:
            self.b.write_bytes(original)

    def test_changed_query_and_pacing_call_destination_are_refused(self):
        original = self.b.read_bytes()
        candidate, failures = audit_manifest(self.b, self.archive, self.manifest, self.runtime, 'fixture')
        self.assertFalse(failures)
        call_displacement = int(candidate['experimental_targets']['inactive_pacing']['image_offset'], 0)+8
        query = original.index(b'vkCreateInstance\0')
        try:
            for offset in (query+2, call_displacement):
                bad = bytearray(original);bad[offset] ^= 1;self.b.write_bytes(bad)
                result = self.native('--derive', self.b, self.archive)
                self.assertEqual(result.returncode, 3, result.stderr.decode())
        finally:
            self.b.write_bytes(original)

    def test_vendor_crlf_stamp_has_the_same_identity(self):
        stamp = self.work/'b/depot/_databuild/databuild.stamp'
        original = stamp.read_bytes()
        baseline = self.derive(self.b)
        try:
            stamp.write_bytes(original.replace(b'\n', b'\r\n'))
            self.assertEqual(self.derive(self.b), baseline)
        finally:
            stamp.write_bytes(original)


if __name__ == '__main__':
    unittest.main()
