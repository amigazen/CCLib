# License

## BSD 2-Clause License

Copyright (c) 2026 amigazen project  
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## Scope and acknowledgment

This repository is a **clean-room reimplementation** of `cclib.library` for
AmigaOS. All original source in this project (under `Source/`, `SDK/`, and
related project files) is new work licensed under the BSD 2-Clause License
above.

The implementation is **inspired by** and intended to be **ABI compatible**
with the historic `cclib.library` by Robert W. Albrecht (Copyright (C)
1989–1990). Compatibility covers calling conventions, library vector offsets
(LVOs), and documented behavior needed to run software written against the
original library.

That acknowledgment does **not** grant any license to Albrecht’s original
`cclib.library` binary, sources, or documentation. The original work remains
under its own copyright and terms. Reference materials kept under `archive/`
(where present) are for historical and ABI study only and are **not** covered by
this BSD 2-Clause License unless a file in that tree states otherwise.
