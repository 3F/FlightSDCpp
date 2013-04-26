:SHA-1: 73fa2e6a9c01f70c09fd2ed7eb965ee59b924580 - StrongDC At revision: 13665.
:SHA-1: 1876dbfc759a9df907dd4083ddbf1648344486ef - base RCF

: Basis for comparison
set basis-cmp-StrongDC=73fa2e6a9c01f70c09fd2ed7eb965ee59b924580
set basis-cmp-RCF=1876dbfc759a9df907dd4083ddbf1648344486ef

: prefix changes
set prefix-ident=%DATE% %TIME%
FOR /F %%i IN ('git rev-parse HEAD') DO set git-cur-sha1=%%i

set ident-content=[ %prefix-ident% :: compared with SHA-1: %git-cur-sha1% ]


: Files
echo [diff: StrongDC++ sqlite]%ident-content% > CHANGES-StrongDC
echo [diff: RCF]%ident-content% > CHANGES-RCF


: StrongDC project
git diff --diff-filter=DMRTUXB --no-prefix %basis-cmp-StrongDC% >> CHANGES-StrongDC

: RCF
git diff --diff-filter=DMRTUXB --relative=RCF/ --src-prefix=RCF/ --dst-prefix=RCF/ %basis-cmp-RCF% >> CHANGES-RCF