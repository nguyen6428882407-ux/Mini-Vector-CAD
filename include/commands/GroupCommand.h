#pragma once
#include "commands/Command.h"
#include <vector>
#include <memory>
#include <cstddef>

class BangVeCanvas;
class GroupShape;

class GroupCommand : public Command {
public:
    enum class Loai {
        GOM,
        RA_NHOM
    };

private:
    BangVeCanvas&               canvas_;
    std::size_t                 layerIdx_;
    Loai                        loai_;
    std::vector<int>            shapeIds_;
    std::vector<std::size_t>    shapePositions_;
    std::unique_ptr<GroupShape> group_;
    int                         groupId_;
    std::size_t                 groupPosition_;
    bool                        daTaoNhomLanthoi_;

public:
    GroupCommand(BangVeCanvas& canvas, std::size_t layerIdx, const std::vector<int>& shapeIds, Loai loai = Loai::GOM);
    GroupCommand(BangVeCanvas& canvas, std::size_t layerIdx, Loai loai, const std::vector<int>& shapeIds);

    GroupCommand(BangVeCanvas& canvas, std::size_t layerIdx, int groupId, Loai loai = Loai::RA_NHOM);

    void execute() override;
    void undo() override;

    Loai getLoai() const;
    int getGroupId() const;
    const std::vector<int>& getShapeIds() const;
};
