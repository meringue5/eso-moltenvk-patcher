# HDE64 provenance

Source: MinHook v1.3.4, commit
`c3fcafdc10146beb5919319d0683e44e3c30d537`, `src/hde/`:
https://github.com/TsudaKageyu/minhook/tree/c3fcafdc10146beb5919319d0683e44e3c30d537/src/hde

Only the byte decoder is used; no MinHook hook installation code is included.
Local portability changes: enable the portable decoder when compiling arm64
use the platform `stdint.h`, use memcpy for unaligned scalar loads, and hide
the decoder symbol/table to avoid exporting them from the Bink proxy. The caller provides padded bounded input and
rejects decoder errors. Unsupported instruction encodings fail closed.
The upstream license is retained and must accompany binary packages.
