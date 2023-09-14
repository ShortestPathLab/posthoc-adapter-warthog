# -*- mode: makefile -*-

# all: main convert extras test
all: main

.PHONY: clean
clean:
	@-$(RM) -rf ./obj/*

main: bin/warthog bin/roadhog bin/mapf

# extras: bin/ch bin/fifo bin/make_cpd

# convert: bin/dimacs2xy bin/dimacs2metis bin/grid2graph

# test: bin/tests test/cpd_search

$(warthog): $(WARTHOG_OBJ)
	@echo "###  Archiving object files: $(warthog) ###"
	@$(shell mkdir -p $(@D))
	@emar -crs $@ $(WARTHOG_OBJ)

$(extra): $(EXTRA_OBJ)
	@echo "###  Archiving object files: $(extra) ###"
	@$(shell mkdir -p $(@D))
	@emar -crs $@ $(EXTRA_OBJ)

-include $(WARTHOG_OBJ:.o=.d)
-include $(EXTRA_OBJ:.o=.d)

# Warthog objects as single targets
obj/%.o: ../../%.cpp
	@$(shell mkdir -p $(@D))
	emcc -c $< -o $@ $(CFLAGS) $(D_WARTHOG_INCLUDES) -MMD -MP -MT $@ -MF $(@:.o=.d)

# Warthog executables as single targets
bin/%: ../../programs/%.cpp $(warthog) $(extra) $(EXTRA_OBJ)
	@echo "linking..."
	@$(shell mkdir -p $(@D))
	emcc $< -o $@ -lwarthog -lwarthog-extra $(CFLAGS) $(D_LIBS) $(D_WARTHOG_INCLUDES)

test/%: ../../test/%.cpp $(warthog) $(extra) $(EXTRA_OBJ)
	@echo "linking..."
	@$(shell mkdir -p $(@D))
	emcc $< -o $@ -lwarthog -lwarthog-extra $(CFLAGS) $(D_LIBS) $(D_WARTHOG_INCLUDES)
