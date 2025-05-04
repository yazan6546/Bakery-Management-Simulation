##
### Add chef-supply-chain test
##add_executable(chef-supply-chain-test
##        chef-supply-chain-test.c
##        ${CMAKE_SOURCE_DIR}/src/inventory.c
##        ${CMAKE_SOURCE_DIR}/src/utils/config.c
##        ${CMAKE_SOURCE_DIR}/src/game.c
##        ${CMAKE_SOURCE_DIR}/src/chefs/chef_utils.c
##        ${CMAKE_SOURCE_DIR}/src/supply_chains/supply_chain_functions.c
##)
##target_link_libraries(chef-supply-chain-test PRIVATE rt pthread) # Add real-time and pthread libraries for shared memory and semaphores
##
##add_executable(inventory-semaphore-test inventory-semaphore-test.c ${CMAKE_SOURCE_DIR}/src/inventory.c)
##target_link_libraries(inventory-semaphore-test PRIVATE rt pthread) # Add real-time and pthread libraries for shared memory and semaphores
#
#add_executable(queue-test queue_test.c ${CMAKE_SOURCE_DIR}/src/utils/products_utils.c
#        ${CMAKE_SOURCE_DIR}/src/utils/random.c
#        ${CMAKE_SOURCE_DIR}/src/utils/shared_mem_utils.c
#        ${CMAKE_SOURCE_DIR}/src/utils/semaphores_utils.c)
#target_include_directories(queue-test PRIVATE ${CMAKE_SOURCE_DIR}/include/lib/queue) # Add the source files for each executable to the main executable
#target_link_libraries(queue-test PRIVATE ${LIBRARY_DIR}/libgenericQueue.a rt) # Add dependencies for the main executable


add_executable(test-shm test-shm.c ${CMAKE_SOURCE_DIR}/src/utils/shared_mem_utils.c
        ${CMAKE_SOURCE_DIR}/src/utils/semaphores_utils.c
        ${CMAKE_SOURCE_DIR}/src/utils/random.c
        ${CMAKE_SOURCE_DIR}/src/utils/products_utils.c
        ${CMAKE_SOURCE_DIR}/src/utils/config.c)

target_include_directories(test-shm PRIVATE ${CMAKE_SOURCE_DIR}/include) # Add the source files for each executable to the main executable)
target_include_directories(test-shm PRIVATE ${CMAKE_SOURCE_DIR}/include/lib/queue) # Add the source files for each executable to the main executable)
target_link_libraries(test-shm PRIVATE ${LIBRARY_DIR}/libgenericQueue.a rt) # Add dependencies for the main executable


#find_package(JSON-C REQUIRED)
#
#add_executable(json-test json_test.c ${CMAKE_SOURCE_DIR}/src/utils/products_utils.c
#${CMAKE_SOURCE_DIR}/src/utils/json-config.c)
#target_include_directories(json-test PRIVATE ${CMAKE_SOURCE_DIR}/include) # Add the source files for each executable to the main executable
#target_link_libraries(json-test PRIVATE JSON-C::JSON-C) # Add dependencies for the main
#
#add_executable(test-queue-removal test_queue_removal.c
#${CMAKE_SOURCE_DIR}/src/customers/customer_utils.c
#${CMAKE_SOURCE_DIR}/src/utils/random.c
#        ${CMAKE_SOURCE_DIR}/src/utils/shared_mem_utils.c)
#target_include_directories(test-queue-removal PRIVATE ${CMAKE_SOURCE_DIR}/include/lib/queue) # Add the source files for each executable to the main executable
#target_link_libraries(test-queue-removal PRIVATE ${LIBRARY_DIR}/libgenericQueue.a rt) # Add dependencies for the main executable
#
#
#add_executable(test_customer test_customer.c ${CMAKE_SOURCE_DIR}/src/customers/customer_utils.c ${CMAKE_SOURCE_DIR}/src/utils/random.c
#        ${CMAKE_SOURCE_DIR}/src/utils/shared_mem_utils.c ${CMAKE_SOURCE_DIR}/src/utils/config.c)
#target_include_directories(test_customer PRIVATE ${CMAKE_SOURCE_DIR}/include) # Add the source files for each executable to the main executable
#target_link_libraries(test_customer PRIVATE ${LIBRARY_DIR}/libgenericQueue.a rt) # Add dependencies for the main executable
#target_include_directories(test_customer PRIVATE ${CMAKE_SOURCE_DIR}/include/lib/queue) # Add the source files for each executable to the main executable
## Add the generator expression as a compile definition
#target_compile_definitions(test_customer PRIVATE
#        CUSTOMERS_BINARY_PATH="$<TARGET_FILE:customers>")