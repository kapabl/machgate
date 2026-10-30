// Darwin-only ASan image-globals surface for Linux providers.
//
// Mach-O instrumented guests register their ASan globals through
// __asan_register_image_globals, which the Linux compiler-rt runtime
// deliberately leaves unimplemented because ELF objects use the
// __asan_register_elf_globals path instead. This adapter exports correct
// no-ops for the Mach-O-only entry points; every other guest ASan import
// resolves through this provider's DT_NEEDED dependency on the real Linux
// runtime. Registered image globals are not poisoned, so global-buffer
// detection is unavailable for such guests while all other checks work.

void __asan_register_image_globals(unsigned long *flag)
{
	(void)flag;
}

void __asan_unregister_image_globals(void)
{
}
