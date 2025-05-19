#pragma once
#include "utility.hpp"

/**
 * @brief API to sync asserted and state properties of a fault LED of a FRU.
 *
 * This API syncs asserted and state properties of a fault LED of a FRU
 * according to the functional property of the FRU.
 *
 * @param[in] i_objectPath - Object path of FRU
 *
 * @throw std::runtime_error
 */
void doSyncFaultLed(const std::string& i_objectPath)
{
    try
    {
        // extract the FRU name from object path
        const auto l_lastSlashPos{i_objectPath.find_last_of('/')};
        if (l_lastSlashPos == std::string::npos)
        {
            throw std::runtime_error("Invalid object path");
        }

        const std::string l_fruName = i_objectPath.substr(l_lastSlashPos + 1);
        std::cout << "FRU name: " << l_fruName << std::endl;

        // lambda to get the Functional property of the FRU
        auto l_getFunctionalProperty =
            [&i_objectPath =
                 std::as_const(i_objectPath)]() -> std::optional<bool> {
            std::optional<bool> l_result;

            const auto l_functionalProperty = utility::getProperty(
                "xyz.openbmc_project.Inventory.Manager",
                "/xyz/openbmc_project/inventory" + i_objectPath,
                "xyz.openbmc_project.State.Decorator.OperationalStatus",
                "Functional");

            if (const auto l_functionalPropertyVal =
                    std::get_if<bool>(&l_functionalProperty))
            {
                l_result.emplace(*l_functionalPropertyVal);
            }
            return l_result;
        };

        // lambda to get the Asserted property of the fault LED
        auto l_getAssertedProperty =
            [](const std::string& i_fruName) -> std::optional<bool> {
            std::optional<bool> l_result;

            const auto l_assertedProperty = utility::getProperty(
                "xyz.openbmc_project.LED.GroupManager",
                "/xyz/openbmc_project/led/groups/" + i_fruName + "_fault",
                "xyz.openbmc_project.Led.Group", "Asserted");

            if (const auto l_assertedPropertyValue =
                    std::get_if<bool>(&l_assertedProperty))
            {
                l_result.emplace(*l_assertedPropertyValue);
            }
            return l_result;
        };

        // lambda to get the physical LED state
        auto l_getPhysicalLedState =
            [](const std::string& i_fruName) -> std::optional<std::string> {
            std::optional<std::string> l_result;

            const auto l_assertedProperty = utility::getProperty(
                "xyz.openbmc_project.LED.Controller.pca955x_" + i_fruName,
                "/xyz/openbmc_project/led/physical/pca955x_" + i_fruName,
                "xyz.openbmc_project.Led.Physical", "State");

            if (const auto l_assertedPropertyValue =
                    std::get_if<std::string>(&l_assertedProperty))
            {
                l_result.emplace(*l_assertedPropertyValue);
            }
            return l_result;
        };

        // lambda to set the Asserted property of the fault LED
        auto l_setAssertedProperty = [](const std::string& i_fruName,
                                        const bool i_value) -> bool {
            return utility::setProperty<bool>(
                "xyz.openbmc_project.LED.GroupManager",
                "/xyz/openbmc_project/led/groups/" + i_fruName + "_fault",
                "xyz.openbmc_project.Led.Group", "Asserted", i_value);
        };

        // lambda to set the Physical property of the LED
        auto l_setPhysicalLedState = [](const std::string& i_fruName,
                                        const std::string& i_value) -> bool {
            return utility::setProperty<std::string>(
                "xyz.openbmc_project.LED.Controller.pca955x_" + i_fruName,
                "/xyz/openbmc_project/led/physical/pca955x_" + i_fruName,
                "xyz.openbmc_project.Led.Physical", "State", i_value);
        };

        // get the Functional property from PIM
        const auto l_functionalProperty = l_getFunctionalProperty();
        if (l_functionalProperty.has_value())
        {
            std::cout << "Current Functional value: " << std::boolalpha
                      << l_functionalProperty.value() << std::endl;
            // get the Asserted property of the fault LED
            const auto l_assertedProperty = l_getAssertedProperty(l_fruName);
            if (l_assertedProperty.has_value())
            {
                std::cout << "Current Asserted value: " << std::boolalpha
                          << l_assertedProperty.value() << std::endl;

                // Asserted is inverse of functional status.
                if (l_functionalProperty.value() == l_assertedProperty.value())
                {
                    if (!l_setAssertedProperty(l_fruName,
                                               !l_functionalProperty.value()))
                    {
                        throw std::runtime_error(
                            "Failed to update Asserted property");
                    }
                    std::cout << "Asserted value updated to: " << std::boolalpha
                              << !l_functionalProperty.value() << std::endl;
                }

                // get the physical LED state
                const auto l_physicalLedState =
                    l_getPhysicalLedState(l_fruName);
                if (l_physicalLedState.has_value())
                {
                    std::cout << "Current Physical LED state: "
                              << l_physicalLedState.value() << std::endl;

                    // if LED is currently blinking, it means identify has been
                    // set, which has higher precedence, so we do nothing.
                    if (l_physicalLedState.value() !=
                        "xyz.openbmc_project.Led.Physical.Action.Blink")
                    {
                        if (l_functionalProperty.value() &&
                            l_physicalLedState.value() ==
                                "xyz.openbmc_project.Led.Physical.Action.On")
                        {
                            // turn off physical LED as FRU is functional
                            if (!l_setPhysicalLedState(
                                    l_fruName,
                                    "xyz.openbmc_project.Led.Physical.Action.Off"))
                            {
                                throw std::runtime_error(
                                    "Failed to turn off physical LED");
                            }
                            std::cout << "Physical LED turned off" << std::endl;
                        }
                        else if (
                            !l_functionalProperty.value() &&
                            l_physicalLedState.value() ==
                                "xyz.openbmc_project.Led.Physical.Action.Off")
                        {
                            // turn on physical LED as FRU is not functional
                            if (!l_setPhysicalLedState(
                                    l_fruName,
                                    "xyz.openbmc_project.Led.Physical.Action.On"))
                            {
                                throw std::runtime_error(
                                    "Failed to turn on physical LED");
                            }
                            std::cout << "Physical LED turned on" << std::endl;
                        }
                    }
                }
                else
                {
                    throw std::runtime_error(
                        "Failed to get physical state of LED");
                }
            }
            else
            {
                throw std::runtime_error(
                    "Failed to get Asserted property value.");
            }
        }
        else
        {
            throw std::runtime_error(
                "Failed to get Functional property value.");
        }
    }
    catch (const std::exception& l_ex)
    {
        throw std::runtime_error(
            "Failed to sync fault LED properties for FRU [" + i_objectPath +
            "]. Error: " + std::string(l_ex.what()));
    }
}
