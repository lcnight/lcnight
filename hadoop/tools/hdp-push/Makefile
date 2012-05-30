JAVA_BIN	=	/usr/bin
JAVAC		=	$(JAVA_BIN)/javac
JAR			=	$(JAVA_BIN)/jar
HADOOP_PATH	=	/home/lc/hadoop/hadoop-1.0.1
CLASSPATH	=	$(HADOOP_PATH)/*:$(HADOOP_PATH)/lib/*
SRC			=	$(wildcard *.java)
CLS			=	$(wildcard *.class)
#CLS			=	$(patsubst %.java, %.class, $(SRC))

hdppush: $(SRC) 
	$(JAVAC) -cp $(CLASSPATH) $(SRC)

tar:
	$(JAR) cvf hdppush.jar *.class

.PHONY: cmd tar r c 

c: clean

r: clean main

clean:
	@rm -rvf *.class *.jar

cmd:
	@echo $(SRC)
