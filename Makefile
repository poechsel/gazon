CC := g++
CFLAGS := -Wall -Wextra -g -fno-stack-protector -z execstack -lpthread -std=c++11 -Wformat-security
SRCDIR := src
BUILDDIR := bin

SRCEXT := cpp
SOURCESSERVER := $(shell find  $(SRCDIR) -not -path "src/client/*" -type f -name '*.$(SRCEXT)')
OBJECTSSERVER := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/obj/%,$(SOURCESSERVER:.$(SRCEXT)=.o))
SOURCESCLIENT := $(shell find  $(SRCDIR) -not -path "src/server/*" -type f -name '*.$(SRCEXT)')
OBJECTSCLIENT := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/obj/%,$(SOURCESCLIENT:.$(SRCEXT)=.o))
DEPSSERVER := $(OBJECTSSERVER:%.o=%.d)
DIRSSERVER := $(dir $(OBJECTSSERVER))
DEPSCLIENT := $(OBJECTSCLIENT:%.o=%.d)
DIRSCLIENT := $(dir $(OBJECTSCLIENT))
LIB := -pthread -L lib
INC := -I include/

all: $(BUILDDIR)/client $(BUILDDIR)/server
	@echo $(OBJECTSSERVER)

$(BUILDDIR)/server: $(OBJECTSSERVER)
	@echo "$(CC) $^ -o $(BUILDDIR)/server $(LIB)"; $(CC) $^ -o $(BUILDDIR)/server $(LIB)

$(BUILDDIR)/client: $(OBJECTSCLIENT)
	@echo "$(CC) $^ -o $(BUILDDIR)/client $(LIB)"; $(CC) $^ -o $(BUILDDIR)/client $(LIB)

-include $(DEPSSERVER)
-include $(DEPSCLIENT)

$(BUILDDIR)/obj/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR) $(DIRSSERVER) $(DIRSCLIENT)
	@echo "$(CC) $(CFLAGS) $(INC) -MMD -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -MMD -c -o $@ $<

clean:
	@echo "Cleaning temporary files."; 
	@echo "$(RM) -r $(BUILDDIR) $(BUILDDIR)/client $(BUILDDIR)/server"; $(RM) -r $(BUILDDIR) $(BUILDDIR)/client $(BUILDDIR)/server

.PHONY: clean
