#pragma once
#include "commands/Command.h"
#include <vector>
#include <memory>
#include <cstddef>

class BangVeCanvas;
class GroupShape;

// ============================================================================
// GroupCommand - Lenh gom nhom va ra nhom hinh hoc (Command Pattern)
// Ho tro 2 che do:
//  1. GOM: Gom cac hinh rieng le (theo shapeIds) thanh mot GroupShape.
//  2. RA_NHOM: Ra mot GroupShape thanh cac hinh thanh phan rieng le trong Layer.
// Hop dong quan trong:
//  - ID ben: ID cua nhom va ID cac hinh con duoc bao toan tuyet doi qua Undo/Redo.
//  - Khong clone hinh trong undo/redo de tranh sinh ID moi va lang phi bo nho.
// ============================================================================
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
    std::unique_ptr<GroupShape> group_;
    int                         groupId_;
    bool                        daTaoNhomLanthoi_;

public:
    // Gom nhom: mac dinh loai = GOM
    GroupCommand(BangVeCanvas& canvas, std::size_t layerIdx, const std::vector<int>& shapeIds, Loai loai = Loai::GOM);

    // Ra nhom: truyen groupId cua nhom can ra
    GroupCommand(BangVeCanvas& canvas, std::size_t layerIdx, int groupId, Loai loai = Loai::RA_NHOM);

    void execute() override;
    void undo() override;

    Loai getLoai() const;
    int getGroupId() const;
    const std::vector<int>& getShapeIds() const;
};
