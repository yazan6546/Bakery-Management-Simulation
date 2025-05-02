#
## Add chef-supply-chain test
#add_executable(chef-supply-chain-test
#        chef-supply-chain-test.c
#        ${CMAKE_SOURCE_DIR}/src/inventory.c
#        ${CMAKE_SOURCE_DIR}/src/utils/config.c
#        ${CMAKE_SOURCE_DIR}/src/game.c
#        ${CMAKE_SOURCE_DIR}/src/chefs/chef_functions.c
#        ${CMAKE_SOURCE_DIR}/src/supply_chains/supply_chain_functions.c
#)
#target_link_libraries(chef-supply-chain-test PRIVATE rt pthread) # Add real-time and pthread libraries for shared memory and semaphores
#
#add_executable(inventory-semaphore-test inventory-semaphore-test.c ${CMAKE_SOURCE_DIR}/src/inventory.c)
#target_link_libraries(inventory-semaphore-test PRIVATE rt pthread) # Add real-time and pthread libraries for shared memory and semaphores

add_executable(queue-test queue_test.c)
target_include_directories(queue-test PRIVATE ${CMAKE_SOURCE_DIR}/include/lib/queue) # Add the source files for each executable to the main executable
target_link_libraries(queue-test PRIVATE ${LIBRARY_DIR}/libgenericQueue.a) # Add dependencies for the main executable

add_executable(json-test json_test.c ${CMAKE_SOURCE_DIR}/src/utils/products_utils.c
${CMAKE_SOURCE_DIR}/src/utils/json-config.c)
target_include_directories(json-test PRIVATE ${CMAKE_SOURCE_DIR}/include) # Add the source files for each executable to the main executable
find_package(json-c REQUIRED)
target_link_libraries(json-test PRIVATE json-c::json-c) # Add dependencies for the main executable