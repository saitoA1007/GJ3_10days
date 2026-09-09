#include "InspectorWindow.h"

using namespace GameEngine;

InspectorWindow::InspectorWindow(GameParamEditor* gameParamEditor, TextureManager* textureManager) {
    gameParamEditor_ = gameParamEditor;
    textureManager_ = textureManager;
}

void InspectorWindow::Draw() {
    if (!isActive) return;

    if (!ImGui::Begin("ParameterInspector", &isActive)) {
        ImGui::End();
        return;
    }

    const std::string& selectedPath = gameParamEditor_->GetRootGroupName();

    // グループが選択されていない場合の表示
    if (selectedPath.empty()) {
        ImGui::TextDisabled("No group selected");
        ImGui::End();
        return;
    }

    // パスからルートグループ名を取得
    const std::string rootGroupName = selectedPath.substr(0, selectedPath.find('/'));

    // ルートグループが存在するかチェック
    auto& allGroups = gameParamEditor_->GetAllGroups();
    auto itRoot = allGroups.find(rootGroupName);
    if (itRoot == allGroups.end()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Group not found: %s", rootGroupName.c_str());
        gameParamEditor_->SetRootGroupName("");
        ImGui::End();
        return;
    }

    // 選択したパスをヘッダーに表示
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Group: %s", selectedPath.c_str());
    ImGui::Separator();

    // 保存ボタン
    if (ImGui::Button("Save")) {
        gameParamEditor_->SaveFile(rootGroupName);
        std::string message = std::format("{}.json saved.", rootGroupName);
        MessageBoxA(nullptr, message.c_str(), "GameParamEditor", 0);
    }

    ImGui::SameLine();

    // 読み込みボタン
    if (ImGui::Button("Load")) {
        gameParamEditor_->LoadFile(rootGroupName);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ルートグループ全体を再帰的に描画
    DrawGroup(itRoot->second, rootGroupName);

    ImGui::End();
}

void InspectorWindow::DrawGroup(GameParamEditor::Group& group, const std::string& groupPath) {

    if (groupPath == "Energy/PlayingTimeline") {
        DrawEnergyTimelineControls(group);
    }

    // このグループのアイテムを描画
    DrawItems(group, groupPath);

    // サブグループを再帰的に
    for (auto& [childName, childGroup] : group.children) {
        ImGui::PushID(childName.c_str());

        if (ImGui::TreeNode(childName.c_str())) {
            // 再帰
            DrawGroup(childGroup, groupPath + "/" + childName);
            ImGui::TreePop();
        }

        ImGui::PopID();
    }
}

void InspectorWindow::DrawItems(GameParamEditor::Group& group, const std::string& groupPath) {
    if (group.items.empty()) { return; }

    // 優先順位でソート
    std::vector<std::pair<std::string, GameParamEditor::Item*>> sortedItems;
    for (auto& [itemName, item] : group.items) {
        sortedItems.push_back({ itemName, &item });
    }
    std::sort(sortedItems.begin(), sortedItems.end(),
        [](const auto& a, const auto& b) {
            if (a.second->priority != b.second->priority) {
                return a.second->priority < b.second->priority;
            }
            return a.first < b.first;
        }
    );

    // ソート済みの順序で描画
    for (auto& [itemName, itemPtr] : sortedItems) {
		// EventCountは専用の追加・削除UIで扱い、数値ドラッグは表示しない。
		if (groupPath == "Energy/PlayingTimeline" && itemName == "EventCount") {
			continue;
		}

        ImGui::PushID(itemName.c_str());
		if (!DrawEnergyTimelineItem(groupPath, itemName, *itemPtr)) {
			std::visit(DebugParameterVisitor{ itemName, itemPtr->isDirty, textureManager_ }, itemPtr->value);
		}
        ImGui::Separator();
        ImGui::PopID();
    }
}

void InspectorWindow::DrawEnergyTimelineControls(GameParamEditor::Group& group) {
    auto countIt = group.items.find("EventCount");
    if (countIt == group.items.end() || !std::holds_alternative<int32_t>(countIt->second.value)) {
        ImGui::TextDisabled("Timeline is unavailable.");
        return;
    }

    auto& eventCount = std::get<int32_t>(countIt->second.value);
    eventCount = (std::max)(eventCount, 0);
    ImGui::Text("Events: %d", eventCount);
    if (ImGui::Button("Add Event")) {
        ++eventCount;
        countIt->second.isDirty = true;
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(eventCount <= 0);
    if (ImGui::Button("Delete Last")) {
        --eventCount;
        countIt->second.isDirty = true;
    }
    ImGui::EndDisabled();
    ImGui::TextDisabled("Arc Position: 0.0 = Left, 0.5 = Center, 1.0 = Right");
    ImGui::Separator();
}

bool InspectorWindow::DrawEnergyTimelineItem(
    const std::string& groupPath,
    const std::string& itemName,
    GameParamEditor::Item& item) {
    if (!groupPath.starts_with("Energy/PlayingTimeline/Events/Event")) {
        return false;
    }

    if (itemName == "EnergySize" && std::holds_alternative<int32_t>(item.value)) {
        constexpr const char* kEnergySizeNames[] = { "Small", "Medium", "Large", "Special" };
        auto& value = std::get<int32_t>(item.value);
        value = (std::clamp)(value, 0, static_cast<int32_t>(std::size(kEnergySizeNames)) - 1);
        ImGui::Text("Energy Size");
        if (ImGui::Combo(
            "##EnergySize",
            &value,
            kEnergySizeNames,
            static_cast<int>(std::size(kEnergySizeNames)))) {
            item.isDirty = true;
        }
        return true;
    }

    if (itemName == "TimeSeconds" && std::holds_alternative<float>(item.value)) {
        auto& value = std::get<float>(item.value);
        ImGui::Text("Time Seconds");
        if (ImGui::DragFloat("##TimeSeconds", &value, 0.1f, 0.0f, 3600.0f, "%.2f s")) {
            value = (std::max)(value, 0.0f);
            item.isDirty = true;
        }
        return true;
    }

    if (itemName == "ArcPosition" && std::holds_alternative<float>(item.value)) {
        auto& value = std::get<float>(item.value);
        ImGui::Text("Arc Position");
        if (ImGui::SliderFloat("##ArcPosition", &value, 0.0f, 1.0f, "%.3f")) {
            item.isDirty = true;
        }
        return true;
    }

    return false;
}
