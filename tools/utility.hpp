#pragma once
#include <sdbusplus/server.hpp>

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace utility
{

// This covers mostly all the data type supported over Dbus for a property.
// clang-format off
using DbusVariantType = std::variant<
    std::monostate,
    std::vector<std::tuple<std::string, std::string, std::string>>,
    std::vector<std::string>,
    std::vector<double>,
    std::string,
    int64_t,
    uint64_t,
    double,
    int32_t,
    uint32_t,
    int16_t,
    uint16_t,
    uint8_t,
    bool,
    std::vector<uint32_t>,
    std::vector<uint16_t>,
    sdbusplus::message::object_path,
    std::tuple<uint64_t, std::vector<std::tuple<std::string, std::string, double, uint64_t>>>,
    std::vector<std::tuple<std::string, std::string>>,
    std::vector<std::tuple<uint32_t, std::vector<uint32_t>>>,
    std::vector<std::tuple<uint32_t, size_t>>,
    std::vector<std::tuple<sdbusplus::message::object_path, std::string,
                           std::string, std::string>>
>;

/**
 * @brief An API to make GetSubTree mapper call.
 *
 * @return MapperResponse - Response from the API.
 */
// map<inventory_path, map<service_name, vector<interface>>>
using MapperResponse =
    std::map<std::string, std::map<std::string, std::vector<std::string>>>;

MapperResponse
    getObjectSubtreeForInterfaces(const std::string& root, const int32_t depth,
                                  const std::vector<std::string>& interfaces)
{
    auto bus = sdbusplus::bus::new_default();
    auto mapperCall = bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                                          "/xyz/openbmc_project/object_mapper",
                                          "xyz.openbmc_project.ObjectMapper",
                                          "GetSubTree");
    mapperCall.append(root);
    mapperCall.append(depth);
    mapperCall.append(interfaces);

    MapperResponse result = {};

    try
    {
        auto response = bus.call(mapperCall);

        response.read(result);
    }
    catch (const sdbusplus::exception_t& e)
    {
        std::cout << "GetSubtree mapper call failed with exception: "
                  << e.what() << std::endl;
    }
    return result;
}

/**
 * @brief An API to get associated object paths.
 *
 * @param[in] associationPath - The path to look for the association endpoints.
 * @param[in] root - The root of the tree to look into.
 * @param[in] depth - The maximum depth of the tree past the root to search.
 * @param[in] interfaces - Optional list of interfaces to constrain the search.
 * @return ListOfObjectPaths - List of object paths associted.
 */
using ListOfObjectPaths = std::vector<std::string>;
ListOfObjectPaths GetAssociatedSubTreePaths(
    const sdbusplus::message::object_path& associationPath,
    const sdbusplus::message::object_path& root, const int32_t depth,
    const std::vector<std::string>& interfaces)
{
    auto bus = sdbusplus::bus::new_default();
    auto mapperCall = bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                                          "/xyz/openbmc_project/object_mapper",
                                          "xyz.openbmc_project.ObjectMapper",
                                          "GetAssociatedSubTreePaths");
    mapperCall.append(associationPath);
    mapperCall.append(root);
    mapperCall.append(depth);
    mapperCall.append(interfaces);

    ListOfObjectPaths result = {};

    try
    {
        auto response = bus.call(mapperCall);

        response.read(result);
    }
    catch (const sdbusplus::exception_t& e)
    {
        std::cerr
            << "GetAssociatedSubTreePaths mapper call failed with exception: "
            << e.what() << std::endl;
    }
    return result;
}

/**
 * @brief Templated API to set D-Bus property.
 *
 * @param[in] serviceName - D-Bus service name hosting the object path.
 * @param[in] objectPath - D-Bus object path hosting the interface.
 * @param[in] interface - D-Bus interface hosting the property.
 * @param[in] property - D-Bus property to be set.
 * @param[in] value - Value to be set.
 *
 * @return On success, returns true, false otherwise.
 */
template <typename T>
bool setProperty(const std::string& serviceName, const std::string& objectPath,
                 const std::string& interface, const std::string& property,
                 const std::variant<T>& value) noexcept
{
    bool l_rc{true};
    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto method =
            bus.new_method_call(serviceName.c_str(), objectPath.c_str(),
                                "org.freedesktop.DBus.Properties", "Set");

        method.append(interface);
        method.append(property);
        method.append(value);
        bus.call(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Set property: " << property
                  << " failed for path: " << objectPath
                  << " with error: " << e.what() << std::endl;
        l_rc = false;
    }
    return l_rc;
}

/**
 * @brief Templated API to get D-Bus property.
 *
 * @param[in] serviceName - D-Bus service name hosting the object path.
 * @param[in] objectPath - D-Bus object path hosting the interface.
 * @param[in] interface - D-Bus interface hosting the property.
 * @param[in] property - D-Bus property whose value is to be fetched.
 * @return Value of the property.
 */
DbusVariantType getProperty(const std::string& serviceName, const std::string& objectPath,
              const std::string& interface, const std::string& property)
{
    DbusVariantType result;

    if(serviceName.empty() || objectPath.empty() || interface.empty() || property.empty())
    {
        std::cout << "One of the parameters required to make Dbus get call is empty" << std::endl;
        return result;
    }

    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto mapperCall =
        bus.new_method_call(serviceName.c_str(), objectPath.c_str(),
                            "org.freedesktop.DBus.Properties", "Get");
    mapperCall.append(interface);
    mapperCall.append(property);
        auto response = bus.call(mapperCall);

        response.read(result);
    }
    catch (const sdbusplus::exception_t& e)
    {
        std::cerr << "Get property: " << property
                  << " failed for path: " << objectPath
                  << " with error: " << e.what() << std::endl;
    }

    return result;
}

/**
 * @brief An API to check chassis power state.
 *
 * @return bool - True when chassis power state is on, false otherwise.
 */
bool isChassisOn()
{
    auto retVal = getProperty(
        "xyz.openbmc_project.State.Chassis0",
        "/xyz/openbmc_project/state/chassis0",
        "xyz.openbmc_project.State.Chassis", "CurrentPowerState");

    if (auto powerSate = std::get_if<std::string>(&retVal))
    {
        if (*powerSate == "xyz.openbmc_project.State.Chassis.PowerState.On")
        {
            return true;
        }
    }

    // In any error condition assuming chassis is off.
    return false;
}

using ObjectMap =
    std::map<sdbusplus::message::object_path,
             std::map<std::string, std::map<std::string, std::variant<bool>>>>;

/**
 * @brief An API to call PIM
 *
 * This API calls notify method of PIM to update the property value.
 *
 * @param[in] objectMap - Map of object path, interface, property and its value.
 */
void notifyPIM(ObjectMap&& objectMap)
{
    try
    {
        for (const auto& objectKeyValue : objectMap)
        {
            auto objectPath = objectMap.extract(objectKeyValue.first);

            if (objectPath.key().str.find("/xyz/openbmc_project/inventory",
                                          0) != std::string::npos)
            {
                objectPath.key() = objectPath.key().str.replace(
                    0, strlen("/xyz/openbmc_project/inventory"), "");
                objectMap.insert(std::move(objectPath));
            }
        }

        auto bus = sdbusplus::bus::new_default();
        auto pimMsg = bus.new_method_call(
            "xyz.openbmc_project.Inventory.Manager",
            "/xyz/openbmc_project/inventory",
            "xyz.openbmc_project.Inventory.Manager", "Notify");
        pimMsg.append(std::move(objectMap));
        bus.call(pimMsg);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Notify PIM to update property value failed with error: "
                  << e.what() << std::endl;
    }
}

/**
 * @brief API to create a PEL
 *
 * @return true on success, otherwise returns false
 */
bool createPEL(const std::string& i_message) noexcept
{
    bool l_rc{true};
    try
    {
        auto l_bus = sdbusplus::bus::new_default();
        auto l_method =
            l_bus.new_method_call("xyz.openbmc_project.Logging",
                                  "/xyz/openbmc_project/logging",
                                  "xyz.openbmc_project.Logging.Create", "Create");

        const std::map<std::string, std::string> l_additionalData{};
        l_method.append(i_message, "xyz.openbmc_project.Logging.Entry.Level.Warning", l_additionalData);
        l_bus.call(l_method);
    }
    catch(const std::exception& l_ex)
    {
        l_rc = false;
        std::cerr << "Failed to create PEL. Error: " << std::string(l_ex.what()) << std::endl;
    }
    return l_rc;
}
} // namespace utility
