# Samples
set(MCL_SAMPLE bench ecdh random rawbench vote pairing large tri-dh bls_sig she_smpl)
foreach(sample IN ITEMS ${MCL_SAMPLE})
	add_executable(sample_${sample} ${sample}.cpp)
	target_link_libraries(sample_${sample} PRIVATE mcl::mcl)
	set_target_properties(sample_${sample} PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO)
	target_compile_options(sample_${sample} PRIVATE ${MCL_COMPILE_OPTIONS})
	target_compile_definitions(sample_${sample} PRIVATE MCL_DONT_EXPORT)
endforeach()

# C interface Sample
add_executable(sample_pairing_c_min pairing_c.c)
target_link_libraries(sample_pairing_c_min PRIVATE mcl::mclbn384_256)
target_compile_definitions(sample_pairing_c_min PRIVATE MCL_DONT_EXPORT)
