
ifeq ($(SILENT),yes)
.SILENT: $(PROG).elf $(LOADER).elf
endif

$(OBJS_DIR)/%.o : %.c
ifeq ($(SILENT),yes)	
	@echo "Compiling $< ..."
	@$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
else
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
endif

$(OBJS_DIR)/%.o : %.cpp
ifeq ($(SILENT),yes)	
	@echo "Compiling $< ..."
	@$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@
else
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@
endif

