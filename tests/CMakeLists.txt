include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

add_executable(tests main_test.cpp zobrist_test.cpp)
target_link_libraries(tests PRIVATE double-go-lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)

add_executable(torch-smoke-test torch_smoke_test.cpp)
target_link_libraries(torch-smoke-test PRIVATE "${TORCH_LIBRARIES}" GTest::gtest_main)
gtest_discover_tests(torch-smoke-test)

add_executable(model-test model_test.cpp ../src/model.cpp)
target_link_libraries(model-test PRIVATE double-go-lib "${TORCH_LIBRARIES}" GTest::gtest_main)
gtest_discover_tests(model-test)
