Setup and create really big file:

  $ . $TESTDIR/../setup.sh
  $ python3 $TESTDIR/create_big_file_compressed.py $TESTDIR/big_file.txt

Search a gz compressed big file with regex:
  $ $TESTDIR/../../ag -z --search-binary --nocolor --workers=1 --parallel 'hello.*' $TESTDIR/big_file.txt.gz
  33554432:hello1073741824
  67108864:hello2147483648
  100663296:hello3221225472
  134217728:hello4294967296
  167772160:hello5368709120
  201326592:hello6442450944
  234881024:hello7516192768
  268435456:hello

Search a bz2 compressed big file with regex:
  $ $TESTDIR/../../ag -z --search-binary --nocolor --workers=1 --parallel 'hello.*' $TESTDIR/big_file.txt.bz2
  33554432:hello1073741824
  67108864:hello2147483648
  100663296:hello3221225472
  134217728:hello4294967296
  167772160:hello5368709120
  201326592:hello6442450944
  234881024:hello7516192768
  268435456:hello

Search a xz compressed big file with regex:
  $ $TESTDIR/../../ag -z --search-binary --nocolor --workers=1 --parallel 'hello.*' $TESTDIR/big_file.txt.xz
  33554432:hello1073741824
  67108864:hello2147483648
  100663296:hello3221225472
  134217728:hello4294967296
  167772160:hello5368709120
  201326592:hello6442450944
  234881024:hello7516192768
  268435456:hello

