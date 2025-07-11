#include "GameImporterUI.h"
#include "Logger.h"
#include <imgui/imgui.h>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

namespace Nexus {

GameImporterUI::GameImporterUI() : gameImporter_(nullptr) {
    memset(pathBuffer_, 0, sizeof(pathBuffer_));
}

GameImporterUI::~GameImporterUI() {
}

void GameImporterUI::ShowImportWizard(bool* open) {
    if (!*open) return;

    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Game Project Import Wizard", open, ImGuiWindowFlags_NoCollapse)) {
        
        ImGui::Text("Import games and assets from Unity, Unreal Engine, and Godot");
        ImGui::Separator();

        if (ImGui::BeginTabBar("ImportTabs")) {
            if (ImGui::BeginTabItem("Project Selection")) {
                DrawProjectSelection();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Import Settings")) {
                DrawImportSettings();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Advanced")) {
                DrawAdvancedSettings();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Separator();
        
        // Bottom section with import button and progress
        if (wizardState_.importInProgress) {
            DrawProgressBar();
        } else {
            DrawImportButton();
        }

        // Show results button if import completed
        if (wizardState_.lastResult.success || !wizardState_.lastResult.errors.empty()) {
            ImGui::SameLine();
            if (ImGui::Button("View Results")) {
                showResults_ = true;
            }
        }
    }
    ImGui::End();

    // Show additional windows
    if (showResults_) {
        ShowImportResults(&showResults_);
    }
    
    if (showAssetBrowser_) {
        ShowAssetBrowser(&showAssetBrowser_);
    }
}

void GameImporterUI::DrawProjectSelection() {
    ImGui::Text("Select a game project to import:");
    ImGui::Spacing();

    // Project path input
    ImGui::Text("Project Path:");
    ImGui::InputText("##ProjectPath", pathBuffer_, sizeof(pathBuffer_));
    ImGui::SameLine();
    
    if (ImGui::Button("Browse...")) {
        std::string selectedPath = OpenFolderDialog("Select Game Project Folder");
        if (!selectedPath.empty()) {
            strncpy_s(pathBuffer_, selectedPath.c_str(), sizeof(pathBuffer_) - 1);
            wizardState_.projectPath = selectedPath;
            RefreshProjectInfo();
        }
    }

    // Recent projects
    if (!recentProjects_.empty()) {
        ImGui::Spacing();
        ImGui::Text("Recent Projects:");
        for (size_t i = 0; i < recentProjects_.size() && i < 5; ++i) {
            if (ImGui::Selectable(recentProjects_[i].c_str())) {
                strncpy_s(pathBuffer_, recentProjects_[i].c_str(), sizeof(pathBuffer_) - 1);
                wizardState_.projectPath = recentProjects_[i];
                RefreshProjectInfo();
            }
        }
    }

    ImGui::Spacing();
    DrawEngineDetection();
}

void GameImporterUI::DrawEngineDetection() {
    if (wizardState_.projectPath.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No project selected");
        return;
    }

    ImGui::Text("Detected Engine:");
    
    switch (wizardState_.detectedEngine) {
        case GameImporter::EngineType::Unity:
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Unity Project");
            ImGui::Text("Project structure validated successfully");
            break;
            
        case GameImporter::EngineType::UnrealEngine:
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Unreal Engine Project");
            ImGui::Text("Project structure validated successfully");
            break;
            
        case GameImporter::EngineType::Godot:
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Godot Project");
            ImGui::Text("Project structure validated successfully");
            break;
            
        case GameImporter::EngineType::Unknown:
        default:
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "✗ Unknown or Invalid Project");
            ImGui::Text("Please select a valid Unity, Unreal, or Godot project folder");
            break;
    }
}

void GameImporterUI::DrawImportSettings() {
    ImGui::Text("Import Settings:");
    ImGui::Spacing();

    // Output directory
    ImGui::Text("Output Directory:");
    static char outputBuffer[256];
    strncpy_s(outputBuffer, wizardState_.settings.outputDirectory.c_str(), sizeof(outputBuffer) - 1);
    if (ImGui::InputText("##OutputDir", outputBuffer, sizeof(outputBuffer))) {
        wizardState_.settings.outputDirectory = outputBuffer;
    }

    ImGui::Spacing();

    // Asset conversion options
    ImGui::Checkbox("Convert Materials", &wizardState_.settings.convertMaterials);
    ImGui::Checkbox("Convert Scripts", &wizardState_.settings.convertScripts);
    ImGui::Checkbox("Convert Animations", &wizardState_.settings.convertAnimations);
    ImGui::Checkbox("Preserve Hierarchy", &wizardState_.settings.preserveHierarchy);
    ImGui::Checkbox("Optimize Meshes", &wizardState_.settings.optimizeMeshes);

    ImGui::Spacing();

    // Script language selection
    if (wizardState_.settings.convertScripts) {
        ImGui::Text("Convert Scripts To:");
        const char* languages[] = { "C++", "Lua", "Python" };
        const char* languageIds[] = { "cpp", "lua", "python" };
        static int selectedLanguage = 0;
        
        if (ImGui::Combo("##ScriptLanguage", &selectedLanguage, languages, 3)) {
            wizardState_.settings.scriptLanguage = languageIds[selectedLanguage];
        }
    }

    // Scale multiplier
    ImGui::Text("Scale Multiplier:");
    ImGui::SliderFloat("##Scale", &wizardState_.settings.scaleMultiplier, 0.01f, 100.0f, "%.2f");
}

void GameImporterUI::DrawAdvancedSettings() {
    ImGui::Text("Advanced Import Options:");
    ImGui::Spacing();

    ImGui::Checkbox("Generate LODs", &wizardState_.settings.generateLODs);
    
    if (ImGui::CollapsingHeader("Mesh Optimization")) {
        ImGui::Indent();
        ImGui::Text("Mesh optimization settings would go here");
        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Texture Processing")) {
        ImGui::Indent();
        ImGui::Text("Texture processing settings would go here");
        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Audio Processing")) {
        ImGui::Indent();
        ImGui::Text("Audio processing settings would go here");
        ImGui::Unindent();
    }
}

void GameImporterUI::DrawImportButton() {
    bool canImport = !wizardState_.projectPath.empty() && 
                     wizardState_.detectedEngine != GameImporter::EngineType::Unknown &&
                     gameImporter_ != nullptr;

    if (!canImport) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Start Import", ImVec2(120, 30))) {
        StartImport();
    }

    if (!canImport) {
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Please select a valid project first");
    }
}

void GameImporterUI::DrawProgressBar() {
    ImGui::Text("Importing project...");
    ImGui::ProgressBar(wizardState_.importProgress, ImVec2(-1, 0));
    
    if (ImGui::Button("Cancel")) {
        // TODO: Implement import cancellation
        wizardState_.importInProgress = false;
    }
}

void GameImporterUI::ShowImportResults(bool* open) {
    if (!*open) return;

    ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Import Results", open)) {
        
        const auto& result = wizardState_.lastResult;
        
        // Summary
        if (result.success) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Import Completed Successfully");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "✗ Import Failed");
        }
        
        ImGui::Text("Conversion Time: %.2f seconds", result.conversionTime);
        ImGui::Text("Assets Imported: %zu", result.importedAssets.size());
        ImGui::Text("Errors: %zu", result.errors.size());
        ImGui::Text("Warnings: %zu", result.warnings.size());
        
        ImGui::Separator();

        if (ImGui::BeginTabBar("ResultTabs")) {
            if (ImGui::BeginTabItem("Imported Assets")) {
                DrawResultsTable();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Errors")) {
                for (const auto& error : result.errors) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", error.c_str());
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Warnings")) {
                for (const auto& warning : result.warnings) {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Warning: %s", warning.c_str());
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Separator();
        
        if (ImGui::Button("Open Asset Browser")) {
            showAssetBrowser_ = true;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            *open = false;
        }
    }
    ImGui::End();
}

void GameImporterUI::DrawResultsTable() {
    if (ImGui::BeginTable("AssetsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Original Path");
        ImGui::TableSetupColumn("Nexus Path");
        ImGui::TableHeadersRow();

        for (const auto& asset : wizardState_.lastResult.importedAssets) {
            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", asset.name.c_str());
            
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", [&asset]() {
                switch (asset.type) {
                    case GameImporter::AssetType::Scene: return "Scene";
                    case GameImporter::AssetType::Mesh: return "Mesh";
                    case GameImporter::AssetType::Material: return "Material";
                    case GameImporter::AssetType::Texture: return "Texture";
                    case GameImporter::AssetType::Audio: return "Audio";
                    case GameImporter::AssetType::Script: return "Script";
                    case GameImporter::AssetType::Animation: return "Animation";
                    case GameImporter::AssetType::Prefab: return "Prefab";
                    case GameImporter::AssetType::Level: return "Level";
                    default: return "Unknown";
                }
            }());
            
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", asset.originalPath.c_str());
            
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", asset.nexusPath.c_str());
        }
        
        ImGui::EndTable();
    }
}

void GameImporterUI::ShowAssetBrowser(bool* open) {
    if (!*open) return;

    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Imported Asset Browser", open)) {
        ImGui::Text("Browse and preview imported assets");
        ImGui::Separator();

        // Asset browser implementation would go here
        ImGui::Text("Asset browser functionality coming soon...");
        
        if (ImGui::Button("Close")) {
            *open = false;
        }
    }
    ImGui::End();
}

std::string GameImporterUI::OpenFolderDialog(const std::string& title) {
    // Platform-specific folder dialog implementation
    // For now, return empty string - would need platform-specific code
    Logger::Info("Opening folder dialog: " + title);
    return "";
}

void GameImporterUI::RefreshProjectInfo() {
    if (!gameImporter_ || wizardState_.projectPath.empty()) {
        wizardState_.detectedEngine = GameImporter::EngineType::Unknown;
        return;
    }

    wizardState_.detectedEngine = gameImporter_->DetectEngineType(wizardState_.projectPath);
    
    // Add to recent projects if valid
    if (wizardState_.detectedEngine != GameImporter::EngineType::Unknown) {
        auto it = std::find(recentProjects_.begin(), recentProjects_.end(), wizardState_.projectPath);
        if (it != recentProjects_.end()) {
            recentProjects_.erase(it);
        }
        recentProjects_.insert(recentProjects_.begin(), wizardState_.projectPath);
        
        // Keep only last 10 recent projects
        if (recentProjects_.size() > 10) {
            recentProjects_.resize(10);
        }
    }
}

void GameImporterUI::StartImport() {
    if (!gameImporter_) return;

    wizardState_.importInProgress = true;
    wizardState_.importProgress = 0.0f;

    // Start import in a separate thread
    std::thread importThread([this]() {
        try {
            wizardState_.lastResult = gameImporter_->ImportProject(wizardState_.projectPath, wizardState_.settings);
        }
        catch (const std::exception& e) {
            wizardState_.lastResult.success = false;
            wizardState_.lastResult.message = "Import failed with exception: " + std::string(e.what());
            Logger::Error(wizardState_.lastResult.message);
        }
        
        wizardState_.importInProgress = false;
        wizardState_.importProgress = 1.0f;
        
        Logger::Info("Import completed: " + wizardState_.lastResult.message);
    });
    
    importThread.detach();
}

void GameImporterUI::UpdateImportProgress() {
    if (wizardState_.importInProgress) {
        // Simple progress animation
        static float progressTime = 0.0f;
        progressTime += ImGui::GetIO().DeltaTime;
        wizardState_.importProgress = (sin(progressTime) + 1.0f) * 0.5f;
    }
}

} // namespace Nexus