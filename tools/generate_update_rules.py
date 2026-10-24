#!/usr/bin/env python3
"""Compile semantic update invariants, never a proprietary object, into C rules."""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from collections import Counter
from pathlib import Path
from eso_update import archive_member, object_text_layout, semantic_query_shape, semantic_reference_shape
from analyze_vk_calls import find_section, load_sections, image_base, fixup_rebases, unaligned_sources


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    m = json.loads(args.manifest.read_text())
    executable = args.exe.read_bytes()
    if hashlib.sha256(executable).hexdigest() != m['sha256']:
        raise SystemExit('Native rules require the exact statically audited executable')
    legacy = m['analysis']['legacy_moltenvk']
    obj = archive_member(args.archive, legacy['archive_member'])
    if hashlib.sha256(obj).hexdigest() != legacy['object_sha256']:
        raise SystemExit('Embedded object differs from manifest')
    addr, text, _, _ = object_text_layout(obj)
    delta = int(legacy['link_delta'], 0)
    targets = m['targets']
    if len(targets) != 17 or len({t['symbol'] for t in targets}) != 17:
        raise SystemExit('Expected 17 unique targets')
    by_symbol = m['analysis']['external_references']['by_symbol']
    if set(by_symbol) != {t['symbol'] for t in targets}:
        raise SystemExit('Incomplete routing boundary')
    sections = load_sections(args.exe)
    section = find_section(sections, '__TEXT', '__text')
    base = image_base(sections)
    code = executable[section.offset:section.offset+section.size]
    start, end = base+delta+addr, base+delta+addr+len(text)
    raw = []
    def add(source, destination, kind):
        if start <= destination < end and not start <= source < end:
            raw.append((source, destination-base, kind))
    for i in range(len(code)):
        source = section.address+i
        if i+5 <= len(code) and code[i] in (0xe8, 0xe9):
            add(source, source+5+struct.unpack_from('<i', code, i+1)[0], 1 if code[i]==0xe8 else 2)
        if i+7 <= len(code) and 0x48 <= code[i] <= 0x4f and code[i]&8 and code[i+1]==0x8d and code[i+2]&0xc7==5:
            add(source, source+7+struct.unpack_from('<i', code, i+3)[0], 3)
        if i+10 <= len(code) and 0x48 <= code[i] <= 0x4f and code[i]&8 and 0xb8 <= code[i+1] <= 0xbf:
            add(source, struct.unpack_from('<Q', code, i+2)[0], 4)
    rejected = unaligned_sources(args.exe, {source for source, _, _ in raw})
    native_counts = Counter((target, kind) for source, target, kind in raw if source not in rejected)
    for source, target in fixup_rebases(args.exe):
        if start <= target < end and not start <= source < end:
            native_counts[(target-base, 5)] += 1
    invariants = {
        'object': legacy['archive_members'],
        'references': semantic_reference_shape(m['analysis']['external_references']),
        'queries': semantic_query_shape(m['analysis']['proc_queries']),
        'targets': [(t['symbol'], int(t['image_offset'], 0)-delta, t['expected_bytes']) for t in targets],
        'runtime': m['analysis']['replacement_runtime']['sha256'],
        'original': m['original_bink_sha256'],
        'renamed': m['retagged_original_bink_sha256'],
        'schema': 1,
        'native_references': [(target-delta, kind, count) for (target, kind), count in sorted(native_counts.items())],
    }
    rules_id = hashlib.sha256(json.dumps(invariants, sort_keys=True).encode()).hexdigest()
    def byte_list(value: str) -> str:
        return ', '.join(f'0x{b:02x}' for b in bytes.fromhex(value))
    lines = ['/* Generated semantic rules; no proprietary object/text is included. */', '#pragma once']
    for key, value in {
        'COMPAT_RULE_ID': rules_id,
        'COMPAT_ARCHIVE_SHA': hashlib.sha256(args.archive.read_bytes()).hexdigest(),
        'COMPAT_OBJECT_SHA': legacy['object_sha256'],
        'COMPAT_OBJECT_MEMBER': legacy['archive_member'],
        'COMPAT_RUNTIME_SHA': invariants['runtime'],
        'COMPAT_ORIGINAL_SHA': invariants['original'],
        'COMPAT_RENAMED_SHA': invariants['renamed'],
    }.items():
        lines.append(f'#define {key} {json.dumps(value)}')
    lines += [f'#define COMPAT_OLD_TEXT_START 0x{delta+addr:x}ULL',
              f'#define COMPAT_OLD_TEXT_END 0x{delta+addr+len(text):x}ULL',
              f'#define COMPAT_OBJECT_TEXT_ADDRESS 0x{addr:x}ULL',
              f'#define COMPAT_BASE_DELTA 0x{delta:x}ULL',
              f'#define COMPAT_PATCH_COUNT {len(targets)}',
              'static const CompatPatch kCompatPatches[COMPAT_PATCH_COUNT] = {']
    lines += [f'    {{ {t["image_offset"]}ULL, {{{byte_list(t["expected_bytes"])}}} }},' for t in targets]
    lines += ['};', 'static const char *const kCompatNames[COMPAT_PATCH_COUNT] = {']
    lines += [f'    {json.dumps(t["symbol"])},' for t in targets]
    refs = [(f'0x{target:x}', kind, count) for (target, kind), count in sorted(native_counts.items())]
    lines += ['};', f'#define COMPAT_REFERENCE_COUNT {len(refs)}', 'static const CompatReference kCompatReferences[COMPAT_REFERENCE_COUNT] = {']
    lines += [f'    {{{offset}ULL, {kind}, {count}}},' for offset, kind, count in refs]
    queries = [(route, name, count) for route, label in [(1, 'GIPA'), (2, 'GDPA')] for name, count in sorted(Counter(item.split(':', 1)[1] for item in m['analysis']['proc_queries']['routes'][label]['queries']).items())]
    lines += ['};', f'#define COMPAT_QUERY_COUNT {len(queries)}', 'static const CompatQuery kCompatQueries[COMPAT_QUERY_COUNT] = {']
    lines += [f'    {{{route}, {json.dumps(name)}, {count}}},' for route, name, count in queries]
    lines += ['};', '']
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text('\n'.join(lines), encoding='ascii')


if __name__ == '__main__':
    main()
