# version component — generated from cmake/version.{hpp,cpp}.in + VERSION file.
# Edit the root VERSION file only; do not hand-edit the generated sources.
target_sources(${PROJECT_NAME}_lib
  PRIVATE
    ${ASCEND_FORGE_GENERATED_DIR}/version.cpp
)
