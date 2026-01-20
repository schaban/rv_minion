#!/bin/sh

OUT_DIR=${OUT_DIR:-out}
PROJ_NAME=${PROJ_NAME:-test}

ZIG=${ZIG:-zig}
_OBJCOPY_=llvm-objcopy
_OBJDUMP_=llvm-objdump
_READELF_=llvm-readelf

mkdir -p $OUT_DIR
$ZIG version

$ZIG build-exe -target riscv32-freestanding -mcpu=generic_rv32+m+d -OReleaseSafe $PROJ_NAME.zig -femit-bin=$OUT_DIR/$PROJ_NAME.elf -T test_zig.ld $*
$_OBJCOPY_ -O binary $OUT_DIR/$PROJ_NAME.elf $OUT_DIR/$PROJ_NAME.bin
$_OBJDUMP_ -d $OUT_DIR/$PROJ_NAME.elf > $OUT_DIR/$PROJ_NAME.txt
$_READELF_ -s -S --wide $OUT_DIR/$PROJ_NAME.elf | tail -n +5 > $OUT_DIR/$PROJ_NAME.info

cat $OUT_DIR/$PROJ_NAME.info | awk '$4 == "FUNC" {print $2 " " $3 " " $8}' > $OUT_DIR/$PROJ_NAME.func
cat $OUT_DIR/$PROJ_NAME.info | awk '$3 == ".text" {print "$code " $5}' > $OUT_DIR/$PROJ_NAME.code
cat $OUT_DIR/$PROJ_NAME.info | awk '$3 == ".data" {print "$data " $5}' > $OUT_DIR/$PROJ_NAME.data
cat $OUT_DIR/$PROJ_NAME.info | awk '$3 == ".sdata" {print "$sdata " $5}' > $OUT_DIR/$PROJ_NAME.sdata

CODE_SIZE=`ls -l $OUT_DIR/$PROJ_NAME.bin | awk '{print $5}'`
NUM_FUNC=`wc -l $OUT_DIR/$PROJ_NAME.func | awk '{print $1}'`

HEAD_FILE=$OUT_DIR/$PROJ_NAME.head
echo 'MINION 1' > $HEAD_FILE
cat $OUT_DIR/$PROJ_NAME.code >> $HEAD_FILE
cat $OUT_DIR/$PROJ_NAME.data >> $HEAD_FILE
cat $OUT_DIR/$PROJ_NAME.sdata >> $HEAD_FILE

echo '$funcs' $NUM_FUNC >> $HEAD_FILE
cat $OUT_DIR/$PROJ_NAME.func >> $HEAD_FILE
echo '$bin' $CODE_SIZE >> $HEAD_FILE

cat $HEAD_FILE $OUT_DIR/$PROJ_NAME.bin > $OUT_DIR/$PROJ_NAME.minion
