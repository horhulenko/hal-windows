// Gives the otherwise empty hal_core DLL an object file, which MSVC needs to produce a DLL and its import library.
namespace hal
{
    void core_library_anchor() {}
}    // namespace hal
