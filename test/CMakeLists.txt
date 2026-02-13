configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py)

add_custom_target(mua-test-depends)
add_dependencies(mua-test-depends
  FileCheck
  count
  muac
  not)

add_lit_testsuite(check-mua "Running the mua regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS
    mua-test-depends)

add_lit_testsuites(mua ${CMAKE_CURRENT_SOURCE_DIR} DEPENDS mua-test-depends)
