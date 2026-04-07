file(GLOB_RECURSE FILES CONFIGURE_DEPENDS
    Source/*.cpp
    Source/*.h
    Source/*.mm
)

fusion_filter_platform_files(FILES)
