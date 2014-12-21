#!/usr/bin/env bash

COLT_VER=1.2.0
COLT=colt-${COLT_VER}.jar

ARGS4J_VER=2.0.21
ARGS4J=args4j-${ARGS4J_VER}.jar

if [ ! -e $COLT ]
then
	wget http://central.maven.org/maven2/colt/colt/${COLT_VER}/${COLT}
else
	echo "Colt already present, not downloading."
fi

if [ ! -e $ARGS4J ]
then
	wget http://maven.jenkins-ci.org/content/repositories/releases/args4j/args4j/${ARGS4J_VER}/${ARGS4J}
else
	echo "args4j already present, not downloading."
fi