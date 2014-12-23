#!/usr/bin/env bash

JCOMMANDER_VER=1.47
UNCOMMONS_MATHS_VER=1.2.3

JCOMMANDER=jcommander-${JCOMMANDER_VER}.jar
UNCOMMONS_MATHS=uncommons-maths-${UNCOMMONS_MATHS_VER}.jar

if [ ! -e ${JCOMMANDER} ]
then
	wget http://central.maven.org/maven2/com/beust/jcommander/${JCOMMANDER_VER}/${JCOMMANDER}
else
	echo "JCommander already present, not downloading."
fi

if [ ! -e ${UNCOMMONS_MATHS} ]
then
	wget http://central.maven.org/maven2/io/gatling/uncommons/maths/uncommons-maths/${UNCOMMONS_MATHS_VER}/${UNCOMMONS_MATHS}
else
	echo "Uncommons maths already present, not downloading."
fi