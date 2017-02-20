JAVA_XADIR=../java/src/com/sleepycat/db/xa

JAVA_XASRCS=\
	$(JAVA_XADIR)/DbXAResource.java \
	$(JAVA_XADIR)/DbXid.java

Release/dbxa.jar : $(JAVA_XASRCS)
	@echo compiling Berkeley DB XA classes 
	@javac -g -d ./Release/classes -classpath "$(CLASSPATH);./Release/classes" $(JAVA_XASRCS)
	@echo creating jar file 
	@cd .\Release\classes 
	@jar cf ../dbxa.jar com\sleepycat\db\xa\*.class 
	@echo Java XA build finished 

Debug/dbxa.jar : $(JAVA_XASRCS)
	@echo compiling Berkeley DB XA classes 
	@javac -g -d ./Debug/classes -classpath "$(CLASSPATH);./Debug/classes" $(JAVA_XASRCS)
	@echo creating jar file 
	@cd .\Debug\classes 
	@jar cf ../dbxa.jar com\sleepycat\db\xa\*.class 
	@echo Java XA build finished 
