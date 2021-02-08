#!/bin/sh

# Upload travis generated stuff

getDaysSinceRelease()
{
	printf %04d $(( ( $(date +'%s') - $(date -ud '2015-04-01' +'%s') )/60/60/24 ))
}

DAYSSINCERELEASE=`getDaysSinceRelease`
COMMITHASH=$(git rev-parse --short HEAD)

PREFIX=$DAYSSINCERELEASE-$(date +"%H-%M")
POSTFIX=$TRAVIS_BRANCH-$COMMITHASH

while [ "$1" != "" ]; do
FNAME=$1
FILE_BASE=${FNAME%.*}
FILE_EXT="${FNAME##*.}"
OUTNAME=$PREFIX-$FILE_BASE-$POSTFIX.$FILE_EXT
#echo $FNAME: `curl --upload-file $FNAME https://transfer.sh/$OUTNAME`
echo $FNAME: $(curl -s --upload-file $FNAME https://oshi.at/$OUTNAME/2160 | grep Direct)
shift
done
exit 0
