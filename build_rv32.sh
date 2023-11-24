#!/bin/sh

DEF_XPREFIX=./gcc/cross-riscv32/bin/riscv32-elf
XPREFIX=${XPREFIX:-$DEF_XPREFIX}

OUT_DIR=${OUT_DIR:-out}
PROJ_NAME=${PROJ_NAME:-test}
CODE_ORG=${CODE_ORG-C0000}

_CC_=$XPREFIX-gcc
_LD_=$XPREFIX-ld
_OBJCOPY_=$XPREFIX-objcopy
_OBJDUMP_=$XPREFIX-objdump
_READELF_=$XPREFIX-readelf

mkdir -p $OUT_DIR

BARE_OPTS="-ffreestanding -nostartfiles -nostdlib -fno-builtin -fno-stack-protector -fno-PIC -ffp-contract=off"
FAST_OPTS=${FAST_OPTS-"-ffp-contract=fast -ffast-math"}

$_CC_ $BARE_OPTS $FAST_OPTS -fno-inline -O3 -march=rv32g -c $PROJ_NAME.c -o $OUT_DIR/$PROJ_NAME.o $*
$_LD_ --section-start=.text=$CODE_ORG -nostdlib $OUT_DIR/$PROJ_NAME.o -o $OUT_DIR/$PROJ_NAME.elf
$_OBJCOPY_ -O binary $OUT_DIR/$PROJ_NAME.elf $OUT_DIR/$PROJ_NAME.bin
$_OBJDUMP_ -d $OUT_DIR/$PROJ_NAME.elf > $OUT_DIR/$PROJ_NAME.txt
$_READELF_ -s --wide $OUT_DIR/$PROJ_NAME.elf | tail -n +5 > $OUT_DIR/$PROJ_NAME.info
cat $OUT_DIR/$PROJ_NAME.info | awk '$4 == "FUNC" {print $2 " " $3 " " $8}' > $OUT_DIR/$PROJ_NAME.func
cat $OUT_DIR/$PROJ_NAME.info | awk '$8 == ".text" {print "$code " $2}' > $OUT_DIR/$PROJ_NAME.code
cat $OUT_DIR/$PROJ_NAME.info | awk '$8 == ".data" {print "$data " $2}' > $OUT_DIR/$PROJ_NAME.data
cat $OUT_DIR/$PROJ_NAME.info | awk '$8 == ".sdata" {print "$sdata " $2}' > $OUT_DIR/$PROJ_NAME.sdata
cat $OUT_DIR/$PROJ_NAME.info | awk '$8 == "__global_pointer$" {print "$gp " $2}' > $OUT_DIR/$PROJ_NAME.gp
CODE_SIZE=`ls -l $OUT_DIR/$PROJ_NAME.bin | awk '{print $5}'`
NUM_FUNC=`wc -l $OUT_DIR/$PROJ_NAME.func | awk '{print $1}'`

HEAD_FILE=$OUT_DIR/$PROJ_NAME.head
echo 'MINION 1' > $HEAD_FILE
cat $OUT_DIR/$PROJ_NAME.code >> $HEAD_FILE
cat $OUT_DIR/$PROJ_NAME.data >> $HEAD_FILE
cat $OUT_DIR/$PROJ_NAME.sdata >> $HEAD_FILE
cat $OUT_DIR/$PROJ_NAME.gp >> $HEAD_FILE
echo '$funcs' $NUM_FUNC >> $HEAD_FILE
cat $OUT_DIR/$PROJ_NAME.func >> $HEAD_FILE
echo '$bin' $CODE_SIZE >> $HEAD_FILE

cat $HEAD_FILE $OUT_DIR/$PROJ_NAME.bin > $OUT_DIR/$PROJ_NAME.minion
