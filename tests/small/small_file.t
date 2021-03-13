Setup and create small file:

  $ . $TESTDIR/../setup.sh
  $ python3 $TESTDIR/create_small_file.py $TESTDIR/small_file.txt

Search a small file:

  $ $TESTDIR/../../ag --nocolor --workers=1 --parallel hello $TESTDIR/small_file.txt
  8388608:hello268435456
  16777216:hello536870912
  25165824:hello805306368
  33554432:hello1073741824
  41943040:hello1342177280
  50331648:hello1610612736
  58720256:hello1879048192
  67108864:hello

