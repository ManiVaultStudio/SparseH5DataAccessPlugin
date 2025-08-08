#pragma once

#include <actions/HorizontalGroupAction.h>
#include <actions/TriggerAction.h>

class AddRemoveButtonAction : public mv::gui::HorizontalGroupAction
{
public:

    /**
     * Constructor
     * @param parent Pointer to parent object
     */
    AddRemoveButtonAction(QObject* parent);

    void changeEnabled(bool cont, bool stop)
    {
        _addOptionButton.setEnabled(cont);
        _removeOptionButton.setEnabled(stop);
    }

public: // Action getters

    mv::gui::TriggerAction& getAddOptionButton() { return _addOptionButton; }
    mv::gui::TriggerAction& getRemoveOptionButton() { return _removeOptionButton; }

private:
    mv::gui::TriggerAction   _addOptionButton;      /** Add Option action */
    mv::gui::TriggerAction   _removeOptionButton;   /** Remove Option action */
};