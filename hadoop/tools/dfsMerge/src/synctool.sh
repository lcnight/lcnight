#! /bin/bash
# change current directory to absolute path
cd `cd $(dirname $0) && pwd`

#export PATH=$HADOOP_INSTALL/bin:$PATH
export HADOOP_INSTALL=~/hadoop/hadoop-1.0.1
export ZOOKEEPER_HOME=~/hadoop/repo/zookeeper
export CLASSPATH=./*::$ZOOKEEPER_HOME/*:$HADOOP_INSTALL/*:$HADOOP_INSTALL/lib/*:$HADOOP_INSTALL/conf

java dfsMerge $@
