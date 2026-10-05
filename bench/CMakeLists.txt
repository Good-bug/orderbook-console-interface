add_executable(spsc_throughput spsc_throughput.cpp)
target_link_libraries(spsc_throughput PRIVATE fh::fh benchmark::benchmark Threads::Threads)

# Latency bench needs HdrHistogram_c. Not in vcpkg.json yet: check the port name
# with `vcpkg search hdr` and the exported CMake target before enabling.
find_package(hdr_histogram CONFIG QUIET)
if(hdr_histogram_FOUND)
  add_executable(spsc_latency spsc_latency.cpp)
  if(TARGET hdr_histogram::hdr_histogram_static)
    set(FH_HDR_TARGET hdr_histogram::hdr_histogram_static)
  else()
    set(FH_HDR_TARGET hdr_histogram::hdr_histogram)
  endif()
  target_link_libraries(spsc_latency PRIVATE fh::fh ${FH_HDR_TARGET} Threads::Threads)
else()
  message(STATUS "hdr_histogram not found: spsc_latency is NOT built")
endif()
