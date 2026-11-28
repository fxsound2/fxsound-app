/* FxSound — AGPL-3.0-or-later. */
#pragma once
#include <cstdint>
#include <string>

// UI-owned, session-only intent after a transient loss. It is not an output pin.
class OutputRecovery
{
public:
    static bool holdExpired(bool sleeping, int& ticks, int interval)
    { return !sleeping && ++ticks >= interval; }
    void userSelected() { ++selectionEpoch_; clear(); }
    void rememberLost(const std::wstring& id)
    {
        if (wanted_.empty() && !id.empty()) { wanted_ = id; rememberedEpoch_ = selectionEpoch_; attempted_ = false; }
    }
    bool pending() const { return !wanted_.empty() && rememberedEpoch_ == selectionEpoch_; }
    const std::wstring& wanted() const { return wanted_; }
    bool requestRestore(std::uint64_t catalogueEpoch)
    {
        if (!pending() || (attempted_ && attemptedCatalogue_ == catalogueEpoch)) return false;
        attempted_ = true; attemptedCatalogue_ = catalogueEpoch; return true;
    }
    void committed(const std::wstring& id) { if (pending() && wanted_ == id) clear(); }
    void clear() { wanted_.clear(); attempted_ = false; }
private:
    std::wstring wanted_;
    std::uint64_t selectionEpoch_ = 0, rememberedEpoch_ = 0, attemptedCatalogue_ = 0;
    bool attempted_ = false;
};
