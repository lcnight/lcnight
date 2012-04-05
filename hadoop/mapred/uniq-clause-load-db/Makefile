JAVA_BIN	=	/usr/bin
JAVAC		=	$(JAVA_BIN)/javac
JAR			=	$(JAVA_BIN)/jar
HADOOP_PATH	=	/home/lc/hadoop/hadoop-1.0.1
CLASSPATH	=	$(HADOOP_PATH)/*:$(HADOOP_PATH)/lib/*
SRC			=	$(wildcard *.java)
TARGET		=	GetUniqClause

$(TARGET): $(SRC) 
	$(JAVAC) -cp $(CLASSPATH) $(SRC)
	$(JAR) cvf $@.jar *.class

jar: 
	$(JAR) cvf $(TARGET).jar *.class

.PHONY: cmd jar r c 

c: clean

r: clean $(TARGET)

clean:
	@rm -rvf *.class *.jar

cmd:
	@echo $(SRC)
