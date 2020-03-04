-include prod_tool.in

-include Makefile.in

ifeq ($(TOOLCHAIN),GCC)
-include $(APPZONE_MAKEFILE)\MakefileGCC
else
ifeq ($(TOOLCHAIN),RVCT3)
-include $(APPZONE_MAKEFILE)\MakefileRVCT3
endif
endif

