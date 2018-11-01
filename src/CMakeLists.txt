# Boost
find_path(Boost_LOCAL NAMES boost/config.hpp PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../include" NO_DEFAULT_PATH)
if(Boost_LOCAL)
	set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../include")
	find_package(Boost 1.53.0)
endif()
if(NOT Boost_FOUND)
	unset(BOOST_ROOT)
	message(STATUS "Did not find Boost locally; trying globally...")
	message(STATUS "If this doesn't work, run ./get-boost.sh and try again.")
	find_package(Boost 1.53.0 REQUIRED)
endif()
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

if(VCPKG_TOOLCHAIN)
	find_package(lmdb CONFIG REQUIRED)
else()
	find_library(HAS_LMDB NAMES lmdb)
endif()

add_definitions("-DWORD_TYPE=unsigned int" "-DPOSITION_TYPE=unsigned char")

set(FNTBL_HEADERS
	${CMAKE_SOURCE_DIR}/include/AtomicPredicate.h
	${CMAKE_SOURCE_DIR}/include/Constraint.h
	${CMAKE_SOURCE_DIR}/include/ContainsStringPredicate.h
	${CMAKE_SOURCE_DIR}/include/CooccurrencePredicate.h
	${CMAKE_SOURCE_DIR}/include/Dictionary.h
	${CMAKE_SOURCE_DIR}/include/FeatureSequencePredicate.h
	${CMAKE_SOURCE_DIR}/include/FeatureSetPredicate.h
	${CMAKE_SOURCE_DIR}/include/Node.h
	${CMAKE_SOURCE_DIR}/include/Params.h
	${CMAKE_SOURCE_DIR}/include/Predicate.h
	${CMAKE_SOURCE_DIR}/include/PrefixSuffixAddPredicate.h
	${CMAKE_SOURCE_DIR}/include/PrefixSuffixIdentityPredicate.h
	${CMAKE_SOURCE_DIR}/include/PrefixSuffixPredicate.h
	${CMAKE_SOURCE_DIR}/include/PrefixSuffixRemovePredicate.h
	${CMAKE_SOURCE_DIR}/include/Rule.h
	${CMAKE_SOURCE_DIR}/include/SingleFeaturePredicate.h
	${CMAKE_SOURCE_DIR}/include/SubwordPartPredicate.h
	${CMAKE_SOURCE_DIR}/include/TBLTree.h
	${CMAKE_SOURCE_DIR}/include/Target.h
	${CMAKE_SOURCE_DIR}/include/bit_vector.h
	${CMAKE_SOURCE_DIR}/include/common.h
	${CMAKE_SOURCE_DIR}/include/debug.h
	${CMAKE_SOURCE_DIR}/include/hash_wrapper.h
	${CMAKE_SOURCE_DIR}/include/index.h
	${CMAKE_SOURCE_DIR}/include/indexed_map.h
	${CMAKE_SOURCE_DIR}/include/io.h
	${CMAKE_SOURCE_DIR}/include/line_splitter.h
	${CMAKE_SOURCE_DIR}/include/linear_map.h
	${CMAKE_SOURCE_DIR}/include/m_pair.h
	${CMAKE_SOURCE_DIR}/include/my_bit_vector.h
	${CMAKE_SOURCE_DIR}/include/svector.h
	${CMAKE_SOURCE_DIR}/include/timer.h
	${CMAKE_SOURCE_DIR}/include/trie.h
	${CMAKE_SOURCE_DIR}/include/typedef.h
	)

set(FNTBL_SOURCES
	Constraint.cc
	ContainsStringPredicate.cc
	CooccurrencePredicate.cc
	Dictionary.cc
	Node.cc
	Params.cc
	Predicate.cc
	PrefixSuffixAddPredicate.cc
	Rule.cc
	SubwordPartPredicate.cc
	TBLTree.cc
	Target.cc
	common.cc
	fnTBL.cc
	index.cc
	io.cc
	${FNTBL_HEADERS}
	)

include_directories(
	${LMDB_INCLUDE_DIR}
	"${CMAKE_SOURCE_DIR}/include"
	)

add_executable(fnTBL ${FNTBL_SOURCES})
#target_link_libraries(fnTBL PRIVATE lmdb)

install(TARGETS fnTBL RUNTIME DESTINATION bin)
