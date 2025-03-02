#ifndef GIT_BLAME_FILE_TREE_VIEW
#define GIT_BLAME_FILE_TREE_VIEW

#include "kateprivate_export.h"

class QString;
namespace KTextEditor
{
class MainWindow;
}

/**
 * This class shows the diff tree for a commit.
 */
class CommitView
{
public:
    /**
     * open treeview for commit with @p hash
     * @filePath can be path of any file in the repo
     */
    static void KATE_PRIVATE_EXPORT openCommit(const QString &hash, const QString &path, KTextEditor::MainWindow *mainWindow);
};

#endif
