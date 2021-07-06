/*
* Copyright 2021 Conquer Space
*/
#include "client/systems/sysplanetviewer.h"

#include <noiseutils.h>
#include <noise/noise.h>

#include <string>
#include <map>

#include "client/systems/sysstarsystemrenderer.h"
#include "client/scenes/universescene.h"

#include "common/components/area.h"
#include "common/components/bodies.h"
#include "common/components/name.h"
#include "common/components/orbit.h"
#include "common/components/organizations.h"
#include "common/components/player.h"
#include "common/components/population.h"
#include "common/components/resource.h"
#include "common/components/surface.h"
#include "common/util/utilnumberdisplay.h"
#include "engine/gui.h"

void conquerspace::client::systems::SysPlanetInformation::DisplayPlanet() {
    namespace cqspc = conquerspace::common::components;
    if (!to_see) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x * 0.4f,
                                    ImGui::GetIO().DisplaySize.y * 0.8f),
                             ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.79f,
                                   ImGui::GetIO().DisplaySize.y * 0.55f),
                            ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    std::string planet_name = "Planet";
    if (selected_planet == entt::null) {
        return;
    }
    if (GetApp().GetUniverse().all_of<cqspc::Name>(selected_planet)) {
        planet_name = GetApp().GetUniverse().get<cqspc::Name>(selected_planet).name;
    }
    ImGui::Begin(planet_name.c_str(), &to_see, window_flags);
    switch (view_mode) {
        case ViewMode::PLANET_VIEW:
            PlanetInformationPanel();
            break;
        case ViewMode::CITY_VIEW:
            CityInformationPanel();
            break;
    }
    ImGui::End();
}

void conquerspace::client::systems::SysPlanetInformation::Init() {}

void conquerspace::client::systems::SysPlanetInformation::DoUI(int delta_time) {
    DisplayPlanet();
}

void conquerspace::client::systems::SysPlanetInformation::DoUpdate(int delta_time) {
    // If clicked on a planet, go to the planet
    // Get the thing
    selected_planet = conquerspace::scene::GetCurrentViewingPlanet(GetApp());
    entt::entity mouse_over = GetApp().GetUniverse().
                        view<conquerspace::client::systems::MouseOverEntity>().front();
    if (!ImGui::GetIO().WantCaptureMouse &&
                GetApp().MouseButtonIsReleased(GLFW_MOUSE_BUTTON_LEFT) &&
                mouse_over == selected_planet && !conquerspace::scene::IsGameHalted() &&
                !GetApp().MouseDragged()) {
        to_see = true;
        spdlog::info("Switched entity");
    }
}

void conquerspace::client::systems::SysPlanetInformation::CityInformationPanel() {
    namespace cqspc = conquerspace::common::components;
    if (ImGui::ArrowButton("cityinformationpanel", ImGuiDir_Left)) {
        view_mode = ViewMode::PLANET_VIEW;
    }
    ImGui::SameLine();
    static bool thing = true;
    ImGui::Checkbox("Macroeconomic/Ownership mode", &thing);

    ImGui::TextFmt("{}", GetApp().GetUniverse().
                                            get<cqspc::Name>(selected_city_entity).name);

    if (GetApp().GetUniverse().all_of<cqspc::Settlement>(selected_city_entity)) {
        int size = GetApp().GetUniverse()
                .get<cqspc::Settlement>(selected_city_entity).population.size();
        for (auto b : GetApp().GetUniverse().get<cqspc::Settlement>(
                              selected_city_entity).population) {
            auto& bad_var_name = GetApp().GetUniverse()
                                    .get<cqspc::PopulationSegment>(b);
            ImGui::TextFmt("Population: {}",
                        conquerspace::util::LongToHumanString(bad_var_name.population));
        }
    } else {
        ImGui::TextFmt("No population");
    }

    if (GetApp().GetUniverse().all_of<cqspc::Industry>(selected_city_entity)) {
        size_t industries = GetApp().GetUniverse()
                .get<cqspc::Industry>(selected_city_entity).industries.size();
        if (ImGui::BeginTabBar("CityTabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Industries")) {
                auto &city_industry = GetApp().GetUniverse()
                    .get<cqspc::Industry>(selected_city_entity);

                ImGui::TextFmt("Factories: {}", industries);
                ImGui::BeginChild("salepanel",
                                            ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f
                                            - ImGui::GetStyle().ItemSpacing.y, 260),
                                            true, ImGuiWindowFlags_HorizontalScrollbar |
                                            window_flags);
                ImGui::Text("Services Sector");
                // List all the stuff it produces
                ImGui::Text("GDP:");
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("ManufacturingPanel",
                                            ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f
                                            - ImGui::GetStyle().ItemSpacing.y, 260),
                                            true, ImGuiWindowFlags_HorizontalScrollbar |
                                            window_flags);
                ImGui::Text("Manufactuing Sector");
                // List all the stuff it produces
                ImGui::Text("GDP:");

                cqspc::ResourceLedger input_resources;
                cqspc::ResourceLedger output_resources;
                for (auto thingies : city_industry.industries) {
                    if (GetApp().GetUniverse()
                        .all_of<cqspc::ResourceConverter, cqspc::Factory>(thingies)) {
                        auto& generator =
                            GetApp().GetUniverse().get<cqspc::ResourceConverter>(thingies);
                        auto& recipe = GetApp().GetUniverse().get<cqspc::Recipe>(generator.recipe);
                        input_resources += recipe.input;
                        output_resources += recipe.output;
                    }
                }

                ImGui::Text("Output");
                // Output table
                if (ImGui::BeginTable("output_table", 2, ImGuiTableFlags_Borders |
                                                                        ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Good");
                    ImGui::TableSetupColumn("Amount");
                    ImGui::TableHeadersRow();
                    for (auto iterator = output_resources.begin();
                                                iterator != output_resources.end(); iterator++) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextFmt("{}", GetApp().GetUniverse()
                                    .get<cqspc::Identifier>(iterator->first).identifier);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextFmt("{}", conquerspace::util::LongToHumanString(
                                                        static_cast<int64_t>(iterator->second)));
                    }
                    ImGui::EndTable();
                }

                ImGui::Text("Input");
                if (ImGui::BeginTable("input_table", 2,
                                                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Good");
                    ImGui::TableSetupColumn("Amount");
                    ImGui::TableHeadersRow();
                    for (auto iterator = input_resources.begin();
                                                iterator != input_resources.end(); iterator++) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextFmt("{}", GetApp().GetUniverse().
                                                get<cqspc::Identifier>(iterator->first)
                                                .identifier);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextFmt("{}", conquerspace::util::LongToHumanString
                                                                                (iterator->second));
                    }
                    ImGui::EndTable();
                }
                ImGui::EndChild();

                ImGui::BeginChild("MinePanel", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f
                                                - ImGui::GetStyle().ItemSpacing.y, 260),
                                                true, ImGuiWindowFlags_HorizontalScrollbar |
                                                window_flags);
                ImGui::Text("Mining Sector");
                ImGui::Text("GDP:");
                // Get what resources they are making
                cqspc::ResourceLedger resources;
                for (auto thingies : city_industry.industries) {
                    if (GetApp().GetUniverse()
                        .all_of<cqspc::ResourceGenerator, cqspc::Mine>(thingies)) {
                        auto& generator =
                            GetApp().GetUniverse().get<cqspc::ResourceGenerator>(thingies);
                        resources += generator;
                    }
                }

                // Draw on table
                if (ImGui::BeginTable("table1", 2,
                                            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Good");
                    ImGui::TableSetupColumn("Amount");
                    ImGui::TableHeadersRow();
                    for (auto iterator = resources.begin();
                                                        iterator != resources.end(); iterator++) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);

                        ImGui::TextFmt("{}", GetApp()
                                .GetUniverse().get<cqspc::Identifier>(iterator->first).identifier);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextFmt("{}",
                                        conquerspace::util::LongToHumanString(iterator->second));
                    }
                    ImGui::EndTable();
                }

                ImGui::EndChild();
                ImGui::SameLine();

                ImGui::BeginChild("AgriPanel", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f
                                                       - ImGui::GetStyle().ItemSpacing.y, 260),
                                                       true, ImGuiWindowFlags_HorizontalScrollbar |
                                                       window_flags);
                ImGui::Text("Agriculture Sector");
                ImGui::Text("GDP:");

                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Resources")) {
                // Consolidate resources
                auto &city_industry = GetApp().GetUniverse()
                    .get<cqspc::Industry>(selected_city_entity);
                cqspc::ResourceLedger resources;
                for (auto area : city_industry.industries) {
                    if (GetApp().GetUniverse().all_of<cqspc::ResourceStockpile>(area)) {
                        // Add resources
                        auto& stockpile =
                                        GetApp().GetUniverse().get<cqspc::ResourceStockpile>(area);
                        resources += stockpile;
                    }
                }

                if (ImGui::BeginTable("table1", 2,
                                            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Good");
                    ImGui::TableSetupColumn("Amount");
                    ImGui::TableHeadersRow();
                    for (auto iterator = resources.begin();
                                                        iterator != resources.end(); iterator++) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);

                        ImGui::TextFmt("{}", GetApp().
                                GetUniverse().get<cqspc::Identifier>(iterator->first).identifier);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextFmt("{}",
                                    conquerspace::util::LongToHumanString(iterator->second));
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
}

void conquerspace::client::systems::SysPlanetInformation::PlanetInformationPanel() {
    namespace cqspc = conquerspace::common::components;
    if (GetApp().GetUniverse().all_of<cqspc::Habitation>(selected_planet)) {
        auto& habit = GetApp().GetUniverse().get<cqspc::Habitation>(selected_planet);
        ImGui::TextFmt("Cities: {}", habit.settlements.size());

        ImGui::BeginChild("citylist", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar |
                                            window_flags);
        // List cities
        for (int i = 0; i < habit.settlements.size(); i++) {
            const bool is_selected = (selected_city_index == i);

            entt::entity e = habit.settlements[i];
            std::string name = GetApp().GetUniverse().get<cqspc::Name>(e)
                    .name;
            if (ImGui::Selectable(fmt::format("{}", name).c_str(), is_selected)) {
                // Load city
                selected_city_index = i;
                selected_city_entity = habit.settlements[i];
                view_mode = ViewMode::CITY_VIEW;

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    // See city
                    spdlog::info("Mouse clicked");
                }
            }

            if (ImGui::IsItemActive() || ImGui::IsItemHovered()) {
                ImGui::SetTooltip(fmt::format("{}", name).c_str());
            }
        }
        ImGui::EndChild();
    }
}
