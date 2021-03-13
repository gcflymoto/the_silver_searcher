Setup and create small file:

  $ . $TESTDIR/../setup.sh
  $ python3 $TESTDIR/create_small_file_compressed.py $TESTDIR/small_file.txt

Search a gz compressed small file with regex:
  $ $TESTDIR/../../ag -z --search-binary --nocolor --workers=1 --parallel 'hello.*' $TESTDIR/small_file.txt.gz
  8388608:hello268435456
  16777216:hello536870912
  25165824:hello805306368
  33554432:hello1073741824
  41943040:hello1342177280
  50331648:hello1610612736
  58720256:hello1879048192
  67108864:hello

Search a bz2 compressed small file with regex:
  $ $TESTDIR/../../ag -z --search-binary --nocolor --workers=1 --parallel 'hello.*' $TESTDIR/small_file.txt.bz2
  8388608:hello268435456
  16777216:hello536870912
  25165824:hello805306368
  33554432:hello1073741824
  41943040:hello1342177280
  50331648:hello1610612736
  58720256:hello1879048192
  67108864:hello

Search a xz compressed small file with regex:
  $ $TESTDIR/../../ag -z --search-binary --nocolor --workers=1 --parallel 'hello.*' $TESTDIR/small_file.txt.xz
  8388608:hello268435456
  16777216:hello536870912
  25165824:hello805306368
  33554432:hello1073741824
  41943040:hello1342177280
  50331648:hello1610612736
  58720256:hello1879048192
  67108864:hello

