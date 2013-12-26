call clean_all.bat
cd compiled\settings
call get_customlocations.bat
cd ..
cd ..
call build_strongdc_sqlite.bat
call build_strongdc_sqlite_64.bat
move *.7z "U:\webdav\src-bin-pdb\strongdc-sqlite"
copy changelog-sqlite-svn.txt "U:\webdav\src-bin-pdb"