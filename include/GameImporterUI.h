#pragma once

#include "GameImporter.h"
#include <string>
#include <vector>

namespace Nexus {

/**
 * User Interface for the Game Import System
 * Provides easy-to-use dialogs and wizards for importing projects
 */
class GameImporterUI {
public:
    struct ImportWizardState {
        std::string projectPath;
        GameImporter::EngineType detectedEngine = GameImporter::EngineType::Unknown;
        GameImporter::ImportSettings settings;
        bool showAdvancedOptions = false;
        bool importInProgress = false;
        float importProgress = 0.0f;
        GameImporter::ImportResult lastResult;
    };

public:
    GameImporterUI();
    ~GameImporterUI();

    // Main UI Functions
    void ShowImportWizard(bool* open);
    void ShowImportProgress(bool* open);
    void ShowImportResults(bool* open);
    void ShowAssetBrowser(bool* open);

    // Utility Functions
    void SetGameImporter(GameImporter* importer) { gameImporter_ = importer; }
    bool IsImportInProgress() const { return wizardState_.importInProgress; }
    const GameImporter::ImportResult& GetLastImportResult() const { return wizardState_.lastResult; }

private:
    // UI Helper Functions
    void DrawProjectSelection();
    void DrawEngineDetection();
    void DrawImportSettings();
    void DrawAdvancedSettings();
    void DrawImportButton();
    void DrawProgressBar();
    void DrawResultsTable();
    void DrawAssetPreview(const GameImporter::AssetInfo& asset);

    // File Dialog Helpers
    std::string OpenFolderDialog(const std::string& title);
    void RefreshProjectInfo();
    void StartImport();
    void UpdateImportProgress();

private:
    GameImporter* gameImporter_;
    ImportWizardState wizardState_;
    std::vector<std::string> recentProjects_;
    char pathBuffer_[512];
    bool showResults_ = false;
    bool showAssetBrowser_ = false;
};

} // namespace Nexus