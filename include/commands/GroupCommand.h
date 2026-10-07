#pragma once
#include "commands/Command.h"
#include <vector>
#include <memory>
#include <cstddef>

class BangVeCanvas;
class GroupShape;

class GroupCommand : public Command {
private:
    BangVeCanvas&               canvas_;
    size_t                      layerIdx_;
    std::vector<int>            shapeIds_;
    std::unique_ptr<GroupShape> group_;
    int                         groupId_;
    bool                        daTaoNhomLanthoi_;

public:
    GroupCommand(BangVeCanvas& canvas, size_t layerIdx, const std::vector<int>& shapeIds);
    void execute() override;
    void undo() override;
};
