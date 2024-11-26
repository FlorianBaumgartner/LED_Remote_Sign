from SCons.Script import Import

Import("env")

# Add -frtti only for C++ files
env.Append(CXXFLAGS=["-frtti"])
