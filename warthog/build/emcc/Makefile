# -*- mode: makefile -*-

# all: main convert extras test
all: main

.PHONY: clean
clean:
	@-$(RM) -rf ./obj/*

main: bin/warthog.js bin/roadhog.js bin/mapf.js

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
bin/%.js: ../../programs/%.cpp $(warthog) $(extra) $(EXTRA_OBJ)
	@echo "linking..."
	@$(shell mkdir -p $(@D))
	emcc -sSINGLE_FILE -sEXPORT_ES6 -s ASSERTIONS=0 -s ALLOW_MEMORY_GROWTH=1 -s WASM_MEM_MAX=2048MB -s NO_EXIT_RUNTIME=1 -sEXPORTED_RUNTIME_METHODS=FS,callMain -Os --closure 1 $< -o $@ -lwarthog -lwarthog-extra $(CFLAGS) $(D_LIBS) $(D_WARTHOG_INCLUDES)

test/%: ../../test/%.cpp $(warthog) $(extra) $(EXTRA_OBJ)
	@echo "linking..."
	@$(shell mkdir -p $(@D))
	emcc $< -o $@ -lwarthog -s ALLOW_MEMORY_GROWTH=1 -s WASM_MEM_MAX=2048MB -s NO_EXIT_RUNTIME=1 -lwarthog-extra $(CFLAGS) $(D_LIBS) $(D_WARTHOG_INCLUDES)
