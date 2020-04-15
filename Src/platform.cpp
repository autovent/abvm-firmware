/* prevents the exception handling name demangling code getting pulled in */
namespace __gnu_cxx {
void __verbose_terminate_handler() {}
}  // namespace __gnu_cxx

extern "C" __attribute__((weak)) void __cxa_pure_virtual(void) {}
