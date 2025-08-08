#include "AddRemoveButtonAction.h"

AddRemoveButtonAction::AddRemoveButtonAction(QObject* parent) :
    HorizontalGroupAction(parent, "AddRemoveButtonAction"),
    _addOptionButton(this, "Add"),
    _removeOptionButton(this, "Remove last")
{
    setText("Change #dims");

    addAction(&_addOptionButton);
    addAction(&_removeOptionButton);

    _addOptionButton.setToolTip("Add an option");
    _removeOptionButton.setToolTip("Remove an option");
}

