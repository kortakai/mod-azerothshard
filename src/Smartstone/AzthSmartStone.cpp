#include "AzthSmartStone.h"
#include "Common.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Define.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "GameTime.h"
#include "Item.h"
#include "LootMgr.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Spell.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "MapMgr.h"
#include "Map.h"
#include "Group.h"
#include <limits>
#include "AZTH.h"
#include "Apps.h"
#include <unordered_map>
#include <unordered_set>

enum SmartStoneCommands
{
    SMRTST_BLACK_MARKET=1,
    SMRTST_CHANGE_FACTION=2,
    SMRTST_RENAME=3,
    SMRTST_CHAR_MENU=4,
    SMRTST_CHANGE_RACE=5,
    SMRTST_JUKEBOX=6,
    SMRTST_HERBALISM_BONUS=7,
    SMRTST_MINING_BONUS=8,
    SMRTST_BONUS_MENU=9,
    SMRTST_MAX_SKILL=10,
    SMRTST_XP_CHANGE=11,
    SMRTST_RESET_AURAS=12,
    SMRTST_TELEPORT_DALARAN=13,
    SMRTST_TELEPORT_HOUSE=14,
    SMRTST_SHOP_MENU=2000, //unused
    SMRTST_BACK_MENU=2001,
    SMRTST_README=60402,
    SMRTST_README_CHILD=60400,
    // Keep Premium actions below the database readme menu range (60400+).
    // That handler runs first and otherwise consumes these selections.
    SMRTST_PREMIUM_MENU=59000,
    SMRTST_PREMIUM_BANK=59001,
    SMRTST_PREMIUM_VENDOR=59002,
    SMRTST_PREMIUM_MAIL=59003,
    SMRTST_PREMIUM_REPAIR=59004,
    SMRTST_PREMIUM_WORKSHOP=59005,
    SMRTST_PREMIUM_XP_RATE=59006,
    SMRTST_PREMIUM_RESTED_XP=59007,
    SMRTST_PREMIUM_SANCTUARY=59008,
    SMRTST_PREMIUM_XP_RATE_01=59010,
    SMRTST_PREMIUM_XP_RATE_05=59011,
    SMRTST_PREMIUM_XP_RATE_1=59012,
    SMRTST_PREMIUM_XP_RATE_2=59013,
    SMRTST_PREMIUM_XP_RATE_3=59014,
    SMRTST_PREMIUM_XP_RATE_4=59015,
    SMRTST_PREMIUM_XP_RATE_5=59016,
};

constexpr uint32 AETHRO_SANCTUARY_VENDOR = 900004;
constexpr uint32 AETHRO_LUCKY_ANGLER_ITEM = 900005;
constexpr uint32 FIELD_WORKSHOP_ANVIL = 1744;
constexpr uint32 FIELD_WORKSHOP_FORGE = 1743;
constexpr uint32 FIELD_WORKSHOP_MOONWELL = 19260;
constexpr uint32 FIELD_WORKSHOP_COOKING_FIRE = 1915;
constexpr uint32 FIELD_WORKSHOP_DURATION = 5 * MINUTE;
constexpr uint32 FIELD_WORKSHOP_COOLDOWN = 10 * MINUTE;

std::unordered_map<uint32, time_t> activeWorkshops;
std::unordered_map<uint32, time_t> workshopCooldowns;
std::unordered_set<uint32> premiumRestedXpCharacters;

bool IsPremiumRestedXpEnabled(Player const* player)
{
    QueryResult result = CharacterDatabase.Query(
        "SELECT `enabled` FROM `character_premium_rested_xp` WHERE `guid` = {}", player->GetGUID().GetCounter());
    return result && result->Fetch()[0].Get<uint8>() == 1;
}

void SetPremiumRestedXpEnabled(Player const* player, bool enabled)
{
    CharacterDatabase.Execute(
        "INSERT INTO `character_premium_rested_xp` (`guid`, `enabled`) VALUES ({}, {}) "
        "ON DUPLICATE KEY UPDATE `enabled` = VALUES(`enabled`)", player->GetGUID().GetCounter(), enabled ? 1 : 0);
}

void RefillPremiumRestedXp(Player* player)
{
    // SetRestBonus clamps this to the character's current maximum rested-XP pool.
    player->SetRestBonus(std::numeric_limits<float>::max());
}

float GetSmartStoneXPSelection(uint32 action)
{
    switch (action)
    {
        case SMRTST_PREMIUM_XP_RATE_01: return 0.1f;
        case SMRTST_PREMIUM_XP_RATE_05: return 0.5f;
        case SMRTST_PREMIUM_XP_RATE_1: return 1.0f;
        case SMRTST_PREMIUM_XP_RATE_2: return 2.0f;
        case SMRTST_PREMIUM_XP_RATE_3: return 3.0f;
        case SMRTST_PREMIUM_XP_RATE_4: return 4.0f;
        case SMRTST_PREMIUM_XP_RATE_5: return 5.0f;
        default: return -1.0f;
    }
}

bool SummonPremiumFieldWorkshop(Player* player)
{
    time_t const now = GameTime::GetGameTime().count();
    uint32 const guid = player->GetGUID().GetCounter();
    if (auto const active = activeWorkshops.find(guid); active != activeWorkshops.end() && active->second > now)
    {
        ChatHandler(player->GetSession()).SendSysMessage("Your Premium Field Workshop is already active.");
        return false;
    }
    if (auto const cooldown = workshopCooldowns.find(guid); cooldown != workshopCooldowns.end() && cooldown->second > now)
    {
        ChatHandler(player->GetSession()).SendSysMessage("Your Premium Field Workshop is recharging. Please try again shortly.");
        return false;
    }

    Position pos = player->GetPosition();
    float const orientation = player->GetOrientation();
    player->SummonGameObject(FIELD_WORKSHOP_ANVIL, pos.GetPositionX() + 2.0f, pos.GetPositionY(), pos.GetPositionZ(), orientation, 0.0f, 0.0f, 0.0f, 0.0f, FIELD_WORKSHOP_DURATION);
    player->SummonGameObject(FIELD_WORKSHOP_FORGE, pos.GetPositionX() - 2.0f, pos.GetPositionY(), pos.GetPositionZ(), orientation, 0.0f, 0.0f, 0.0f, 0.0f, FIELD_WORKSHOP_DURATION);
    player->SummonGameObject(FIELD_WORKSHOP_MOONWELL, pos.GetPositionX(), pos.GetPositionY() + 2.5f, pos.GetPositionZ(), orientation, 0.0f, 0.0f, 0.0f, 0.0f, FIELD_WORKSHOP_DURATION);
    player->SummonGameObject(FIELD_WORKSHOP_COOKING_FIRE, pos.GetPositionX(), pos.GetPositionY() - 2.5f, pos.GetPositionZ(), orientation, 0.0f, 0.0f, 0.0f, 0.0f, FIELD_WORKSHOP_DURATION);
    activeWorkshops[guid] = now + FIELD_WORKSHOP_DURATION;
    workshopCooldowns[guid] = now + FIELD_WORKSHOP_COOLDOWN;
    ChatHandler(player->GetSession()).SendSysMessage("Your Premium Field Workshop will remain available for five minutes.");
    return true;
}

bool IsPremiumSmartStoneAccount(Player* player)
{
    if (!player || !player->GetSession())
        return false;

    QueryResult result = LoginDatabase.Query("SELECT `premium` FROM `account` WHERE `id` = {}",
        player->GetSession()->GetAccountId());
    return result && result->Fetch()[0].Get<uint8>() == 1;
}

bool CanUsePremiumSmartStoneService(Player* player)
{
    return player && player->IsAlive() && !player->IsInCombat() && !player->IsInFlight() &&
        !player->InBattleground() && !player->InArena();
}

void SendPremiumSmartStoneDenied(Player* player)
{
    ChatHandler(player->GetSession()).SendSysMessage("This SmartStone service requires an active Aethro Premium account.");
}

bool TeleportToSanctuary(Player* player)
{
    uint32 const map = sConfigMgr->GetOption<uint32>("AethroReforged.SanctuaryStone.Destination.Map", 573);
    float const x = sConfigMgr->GetOption<float>("AethroReforged.SanctuaryStone.Destination.X", 128.44106f);
    float const y = sConfigMgr->GetOption<float>("AethroReforged.SanctuaryStone.Destination.Y", 300.45688f);
    float const z = sConfigMgr->GetOption<float>("AethroReforged.SanctuaryStone.Destination.Z", 0.0023937225f);
    float const orientation = sConfigMgr->GetOption<float>("AethroReforged.SanctuaryStone.Destination.O", 5.033745f);

    if (!player->TeleportTo(map, x, y, z, orientation))
    {
        ChatHandler(player->GetSession()).SendSysMessage("The Sanctuary refuses the path from here.");
        return false;
    }

    ChatHandler(player->GetSession()).SendSysMessage("The Sanctuary opens for you.");
    return true;
}

/*static*/ SmartStone* SmartStone::instance()
{
    static SmartStone instance;
    return &instance;
}

std::string SmartStoneCommand::getText(Player *pl)
{
    AzthCustomLangs loc = AZTH_LOC_IT;

    if (pl)
        loc = sAZTH->GetAZTHPlayer(pl)->getCustLang();

    switch(loc)
    {
        case AZTH_LOC_IT:
            return text_it;
        break;
        case AZTH_LOC_EN:
            return text_def;
        break;
    }

    return text_def;
}

class azth_smart_stone : public ItemScript
{
public:

    uint32 parent = 1;
    SmartStoneApps *apps;

    azth_smart_stone() : ItemScript("azth_smart_stone") {
        apps = new SmartStoneApps();
    }

    Player* getHomeOwner(Player *player) {
        Player *owner=player;
        Player* leader=nullptr;
        if (player->GetGroup())
            leader = ObjectAccessor::FindPlayer(player->GetGroup()->GetLeaderGUID());

        if (leader)
            owner = leader;

        return owner;
    }


    void OnGossipSelect(Player *player, Item *item, uint32  /*sender*/,
            uint32 action) override {

        player->PlayerTalkClass->ClearMenus();

        // hack for readme gossip menu
        if (action>=SMRTST_README_CHILD || player->PlayerTalkClass->GetGossipMenu().GetMenuId() >=SMRTST_README_CHILD) {
            uint32 menuId= action; //action >= SMRTST_README_CHILD ? action : player->PlayerTalkClass->GetGossipMenu().GetItemData(action)->GossipActionMenuId;
            player->PlayerTalkClass->GetGossipMenu().SetMenuId(menuId);

            GossipMenuItemsMapBounds menuItemBounds = sObjectMgr->GetGossipMenuItemsMapBounds(menuId);
            uint32 textId = DEFAULT_GOSSIP_MESSAGE;

            GossipMenusMapBounds menuBounds = sObjectMgr->GetGossipMenusMapBounds(menuId);

            for (GossipMenusContainer::const_iterator itr = menuBounds.first; itr != menuBounds.second; ++itr)
                textId = itr->second.TextID;

            for (GossipMenuItemsContainer::const_iterator itr = menuItemBounds.first; itr != menuItemBounds.second; ++itr)
            {
                std::string strOptionText = itr->second.OptionText;
                std::string strBoxText = itr->second.BoxText;

                int32 locale = player->GetSession()->GetSessionDbLocaleIndex();
                if (locale >= 0)
                {
                    uint32 idxEntry = MAKE_PAIR32(menuId, itr->second.OptionID);
                    if (GossipMenuItemsLocale const* no = sObjectMgr->GetGossipMenuItemsLocale(idxEntry))
                    {
                        ObjectMgr::GetLocaleString(no->OptionText, locale, strOptionText);
                        ObjectMgr::GetLocaleString(no->BoxText, locale, strBoxText);
                    }
                }

                player->PlayerTalkClass->GetGossipMenu().AddMenuItem(itr->second.OptionID, itr->second.OptionIcon, strOptionText, 0, itr->second.ActionMenuID, strBoxText, itr->second.BoxMoney, itr->second.BoxCoded);
                player->PlayerTalkClass->GetGossipMenu().AddGossipMenuItemData(itr->second.OptionID, itr->second.ActionMenuID, itr->second.ActionPoiID);
            }

            SendGossipMenuFor(player, textId, item->GetGUID());
            return;
        }

        // back to main menu command
        if (action == 2001) {
            parent = 1;
            CloseGossipMenuFor(player);
            OnUse(player, item, SpellCastTargets());
            return;
        }

        if (action >= SMRTST_PREMIUM_MENU && action <= SMRTST_PREMIUM_XP_RATE_5) {
            if (!IsPremiumSmartStoneAccount(player)) {
                SendPremiumSmartStoneDenied(player);
                CloseGossipMenuFor(player);
                return;
            }
            if (!CanUsePremiumSmartStoneService(player)) {
                ChatHandler(player->GetSession()).SendSysMessage("This SmartStone service cannot be used in your current state.");
                CloseGossipMenuFor(player);
                return;
            }

            switch (action) {
                case SMRTST_PREMIUM_MENU:
                    parent = SMRTST_PREMIUM_MENU;
                    OnUse(player, item, SpellCastTargets());
                    return;
                case SMRTST_PREMIUM_BANK:
                    player->GetSession()->SendShowBank(player->GetGUID());
                    break;
                case SMRTST_PREMIUM_VENDOR:
                {
                    Position pos = player->GetNearPosition(2.0f, 0.0f);
                    if (Creature* vendor = player->SummonCreature(AETHRO_SANCTUARY_VENDOR, pos,
                        TEMPSUMMON_TIMED_DESPAWN, 90 * IN_MILLISECONDS))
                        player->GetSession()->SendListInventory(vendor->GetGUID());
                    else
                        ChatHandler(player->GetSession()).SendSysMessage("The Premium Vendor is unavailable right now.");
                    break;
                }
                case SMRTST_PREMIUM_MAIL:
                    player->GetSession()->SendShowMailBox(player->GetGUID());
                    break;
                case SMRTST_PREMIUM_REPAIR:
                {
                    uint32 cost = player->DurabilityRepairAll(true, 1.0f, false);
                    if (cost)
                        ChatHandler(player->GetSession()).PSendSysMessage("Equipment repaired for {} copper.", cost);
                    else
                        ChatHandler(player->GetSession()).SendSysMessage("No equipment could be repaired, or you do not have enough gold.");
                    break;
                }
                case SMRTST_PREMIUM_WORKSHOP:
                    SummonPremiumFieldWorkshop(player);
                    break;
                case SMRTST_PREMIUM_XP_RATE:
                    parent = SMRTST_PREMIUM_XP_RATE;
                    OnUse(player, item, SpellCastTargets());
                    return;
                case SMRTST_PREMIUM_RESTED_XP:
                {
                    bool const enabled = !IsPremiumRestedXpEnabled(player);
                    SetPremiumRestedXpEnabled(player, enabled);

                    uint32 const guid = player->GetGUID().GetCounter();
                    if (enabled)
                    {
                        premiumRestedXpCharacters.insert(guid);
                        RefillPremiumRestedXp(player);
                        ChatHandler(player->GetSession()).SendSysMessage("Constant Rested XP enabled.");
                    }
                    else
                    {
                        premiumRestedXpCharacters.erase(guid);
                        player->SetRestBonus(0.0f);
                        ChatHandler(player->GetSession()).SendSysMessage("Constant Rested XP disabled.");
                    }
                    break;
                }
                case SMRTST_PREMIUM_SANCTUARY:
                    TeleportToSanctuary(player);
                    break;
                default:
                {
                    float rate = GetSmartStoneXPSelection(action);
                    if (rate >= 0.0f)
                        sAZTH->GetAZTHPlayer(player)->AzthSelfChangeXp(rate);
                    break;
                }
            }
            CloseGossipMenuFor(player);
            return;
        }

        SmartStoneCommand selectedCommand = sSmartStone->getCommandById(action);

        // scripted action
        if (selectedCommand.type == DO_SCRIPTED_ACTION ||
                action >= 2000) // azeroth store
        {
            switch (action) {
                case SMRTST_SHOP_MENU: // store
                    //sSmartStone->SmartStoneSendListInventory(player->GetSession());
                break;
                case SMRTST_BLACK_MARKET: // black market teleport
                    apps->blackMarketTeleport(player);
                break;

                case SMRTST_CHANGE_FACTION: // change faction
                    apps->changeFaction(player);
                break;

                case SMRTST_RENAME: // rename
                    apps->rename(player);
                break;

                case SMRTST_CHANGE_RACE: // change race
                    apps->changeRace(player);
                break;

                case SMRTST_JUKEBOX: // jukebox
                    apps->jukebox(player);
                break;

                case SMRTST_MAX_SKILL: // maxskill
                    apps->maxSkill(player);
                break;

                case SMRTST_RESET_AURAS:
                    apps->resetAuras(player);
                break;

                case SMRTST_TELEPORT_DALARAN:
                    apps->teleportDalaran(player);
                break;

                case SMRTST_TELEPORT_HOUSE:
                    apps->teleportHouse(getHomeOwner(player), player);
                break;

                case 99999:
                    break;
                default:
                    LOG_ERROR("server", "Smartstone: unhandled command! ID: %u, player GUID: %lu", action, player->GetGUID().GetRawValue());
                    break;
            }
            if (selectedCommand.charges > 0) {
                sAZTH->GetAZTHPlayer(player)->decreaseSmartStoneCommandCharges(
                        selectedCommand.id);
            }
            CloseGossipMenuFor(player);
            // return;
        }

        // open child
        if (selectedCommand.type == OPEN_CHILD) {
            parent = selectedCommand.action;
            CloseGossipMenuFor(player);
            OnUse(player, item, SpellCastTargets());
        }
    }

    void OnGossipSelectCode(Player* player, Item*  /*item*/, uint32  /*sender*/, uint32 action, const char* code) override {
        player->PlayerTalkClass->ClearMenus();

        SmartStoneCommand selectedCommand = sSmartStone->getCommandById(action);

        // scripted action
        if (selectedCommand.type == DO_SCRIPTED_ACTION_WITH_CODE || action == 2000) // azeroth store
        {
            switch (action) {
                case SMRTST_XP_CHANGE: //change exp
                    apps->changeExp(player, code);
                break;

                case 99999:
                    break;
                default:
                    LOG_ERROR("server", "Smartstone: unhandled command with code! ID: %u, player GUID: %lu", action, player->GetGUID().GetRawValue());
                    break;
            }
            if (selectedCommand.charges > 0) {
                sAZTH->GetAZTHPlayer(player)->decreaseSmartStoneCommandCharges(selectedCommand.id);
            }
            CloseGossipMenuFor(player);
        }
    }

    bool OnUse(Player *player, Item *item, SpellCastTargets const & /*targets*/) override
    {
        player->PlayerTalkClass->ClearMenus();

        if (parent == 1) // Aethro-only main menu
        {
            AddGossipItemFor(player, 0, "Premium Services", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_MENU);
        }

        if (parent == SMRTST_PREMIUM_MENU)
        {
            AddGossipItemFor(player, 0, "Personal Bank", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_BANK);
            AddGossipItemFor(player, 0, "Premium Vendor", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_VENDOR);
            AddGossipItemFor(player, 0, "Teleport to Sanctuary", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_SANCTUARY);
            AddGossipItemFor(player, 0, "Mailbox", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_MAIL);
            AddGossipItemFor(player, 0, "Repair Equipment", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_REPAIR);
            AddGossipItemFor(player, 0, "Premium Field Workshop", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_WORKSHOP);
            AddGossipItemFor(player, 0,
                IsPremiumRestedXpEnabled(player) ? "Constant Rested XP: On" : "Constant Rested XP: Off",
                GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_RESTED_XP);
            AddGossipItemFor(player, 0, "Experience Rate", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_XP_RATE);
        }

        if (parent == SMRTST_PREMIUM_XP_RATE)
        {
            AddGossipItemFor(player, 0, "Set experience rate to 0.1x", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_XP_RATE_01);
            AddGossipItemFor(player, 0, "Set experience rate to 0.5x", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_XP_RATE_05);
            AddGossipItemFor(player, 0, "Set experience rate to 1x", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_XP_RATE_1);
            AddGossipItemFor(player, 0, "Set experience rate to 2x", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_XP_RATE_2);
            AddGossipItemFor(player, 0, "Set experience rate to 3x", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_XP_RATE_3);
            AddGossipItemFor(player, 0, "Set experience rate to 4x", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_XP_RATE_4);
            AddGossipItemFor(player, 0, "Set experience rate to 5x", GOSSIP_SENDER_MAIN, SMRTST_PREMIUM_XP_RATE_5);
        }

        std::vector<SmartStonePlayerCommand> & playerCommands =
                sAZTH->GetAZTHPlayer(player)->getSmartStoneCommands();
        int n = playerCommands.size();

        for (int i = 0; i < n; i++) {
            SmartStoneCommand command =
                    sSmartStone->getCommandById(playerCommands[i].id);

            // if expired or no charges
            if ((playerCommands[i].duration <= static_cast<uint32>(time(NULL)) &&
                    playerCommands[i].duration != 0) ||
                    playerCommands[i].charges == 0) {
                sAZTH->GetAZTHPlayer(player)->removeSmartStoneCommand(playerCommands[i], true);
                continue;
            }

            std::string text = command.getText(player);

            if (playerCommands[i].charges != -1)
                text = text + " (" + std::to_string(playerCommands[i].charges) + ")";

            if (playerCommands[i].duration != 0) {
                uint64 timeDiff = playerCommands[i].duration - time(NULL);
                uint64 seconds = timeDiff % 60;
                uint64 minutes = floor(timeDiff / 60);
                uint64 hours = floor(timeDiff / 3600);
                uint64 days = floor(timeDiff / 3600 / 24);
                if (days >= 1) {
                    text = text + " (" + std::to_string(days) + " days)";
                } else {
                    text = text + " (" + std::to_string(hours) + ":" +
                            std::to_string(minutes) + ":" + std::to_string(seconds) + ")";
                }
            }

            if (parent != 1 && parent != SMRTST_PREMIUM_MENU && command.id != 0 && command.parent_menu == parent) {
                if (command.type != DO_SCRIPTED_ACTION_WITH_CODE) {
                    AddGossipItemFor(player,command.icon, text, GOSSIP_SENDER_MAIN, command.id);
                } else {
                    AddGossipItemFor(player,command.icon, text, GOSSIP_SENDER_MAIN, command.id, sAzthLang->get(AZTH_LANG_SS_VALUE, player), 0, true);
                }
            }
        }

        // acquista app

        /*if (parent == 1)
          AddGossipItemFor(player,
              0, "|TInterface/ICONS/INV_Misc_Coin_03:30|t Azeroth Store",
              GOSSIP_SENDER_MAIN, 2000);*/

        if (parent != 1) {
            // back to main menu command
            AddGossipItemFor(player,0, sAzthLang->get(AZTH_LANG_SS_BACK, player), GOSSIP_SENDER_MAIN, 2001);
        }

        SendGossipMenuFor(player,DEFAULT_GOSSIP_MESSAGE, item->GetGUID());

        parent = 1;
        return false;
    }
};

void SmartStone::loadCommands() {
    // initialize count and array
    uint32 count = 0;
    // sHearthstoneMode->hsAchievementTable.clear();

    // TODO: re-enable
    // QueryResult ssCommandsResult = ExtraDatabase.PQuery(
    //         "SELECT id, text_def, text_it, item, icon, parent_menu, type, action, charges, "
    //         "duration FROM smartstone_commands");

    // if (ssCommandsResult) {
    //     do {
    //         SmartStoneCommand command = {};
    //         command.id = (*ssCommandsResult)[0].GetUInt32();
    //         command.text_def = (*ssCommandsResult)[1].GetString();
    //         command.text_it = (*ssCommandsResult)[2].GetString();
    //         command.item = (*ssCommandsResult)[3].GetUInt32();
    //         command.icon = (*ssCommandsResult)[4].GetUInt32();
    //         command.parent_menu = (*ssCommandsResult)[5].GetUInt32();
    //         command.type = (*ssCommandsResult)[6].GetUInt32();
    //         command.action = (*ssCommandsResult)[7].GetUInt32();
    //         command.charges = (*ssCommandsResult)[8].GetInt32();
    //         command.duration = (*ssCommandsResult)[9].GetUInt64();

    //         ssCommands2.push_back(command);

    //         count++;

    //     } while (ssCommandsResult->NextRow());
    // }

    LOG_INFO("server", "Smartstone: loaded %u commands", count);
}

SmartStoneCommand SmartStone::getCommandById(uint32 id)
{
    std::vector<SmartStoneCommand> temp(ssCommands2);
    int n = temp.size();
    for (int i = 0; i < n; i++) {
        if (temp[i].id == id)
            return temp[i];
    }
    return nullCommand;
};

SmartStoneCommand SmartStone::getCommandByItem(uint32 item) {
    std::vector<SmartStoneCommand> temp(ssCommands2);
    int n = temp.size();
    for (int i = 0; i < n; i++) {
        if (temp[i].item == item)
            return temp[i];
    }
    return nullCommand;
};

bool SmartStone::isNullCommand(SmartStoneCommand command) {
    return (command.id == 0 && command.text_def == "" && command.item == 0 &&
            command.icon == 0 && command.parent_menu == 0 &&
            command.type == 0 && command.action == 0);
};

SmartStonePlayerCommand SmartStone::toPlayerCommand(SmartStoneCommand command) {
    SmartStonePlayerCommand result;
    result.id = command.id;
    result.charges = command.charges;
    // result.duration = command.duration * 60 + time(NULL);
    result.duration = 0;
    return result;
};

class azth_smartstone_world : public WorldScript {
public:

    azth_smartstone_world() : WorldScript("azth_smartstone_world") {
    }

    void OnAfterConfigLoad(bool  /*reload*/) override {
        sSmartStone->loadCommands();
    }
};

class azth_smartstone_player_commands : public PlayerScript {
public:

    azth_smartstone_player_commands()
    : PlayerScript("azth_smartstone_player_commands") {
    }

    void OnPlayerLogin(Player *player) override {
        QueryResult ssCommandsResult = CharacterDatabase.Query(
                "SELECT command, dateExpired, charges FROM "
                "character_smartstone_commands WHERE playerGuid = {} ;",
                player->GetGUID().GetCounter());

        if (ssCommandsResult) {
            do {
                uint32 id = (*ssCommandsResult)[0].Get<uint32>();
                uint64 date = (*ssCommandsResult)[1].Get<uint64>();
                int32 charges = (*ssCommandsResult)[2].Get<int32>();
                sAZTH->GetAZTHPlayer(player)->addSmartStoneCommand(id, false, date, charges);
            } while (ssCommandsResult->NextRow());
        }

        sAZTH->GetAZTHPlayer(player)->getLastPositionInfoFromDB();
    }

    void OnPlayerLogout(Player* player) override {
        sAZTH->GetAZTHPlayer(player)->saveLastPositionInfoToDB(player);
    }

    void OnPlayerBeforeBuyItemFromVendor(Player* player, ObjectGuid vendorguid, uint32 vendorslot, uint32 &item, uint8 count, uint8  /*bag*/, uint8 /*slot*/) override {
        if (!sSmartStone->isNullCommand(sSmartStone->getCommandByItem(item))) {
            sAZTH->GetAZTHPlayer(player)->BuySmartStoneCommand(vendorguid, vendorslot, item, count, NULL_BAG, NULL_SLOT);
            item = 0;
        }
    }

};

class smartstone_vendor : public CreatureScript {
public:

    smartstone_vendor() : CreatureScript("smartstone_vendor") {
    }

    bool OnGossipHello(Player* player, Creature* creature) override {
        player->PlayerTalkClass->ClearMenus();

        AddGossipItemFor(player,0, "Hello, I would like to buy new apps!", GOSSIP_SENDER_MAIN, 1);
        SendGossipMenuFor(player,DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override {
        player->PlayerTalkClass->ClearMenus();

        if (action == 1)
            sSmartStone->SmartStoneSendListInventory(player->GetSession(), creature->GetGUID().GetRawValue());

        return true;
    }
};

void SmartStone::SmartStoneSendListInventory(WorldSession *session, uint64 vendorGuid) {
    VendorItemData const *items = sObjectMgr->GetNpcVendorItemList(SMARTSTONE_VENDOR_ENTRY);



    if (!items) {
        WorldPacket data(SMSG_LIST_INVENTORY, 8 + 1 + 1);
        data << vendorGuid;
        data << uint8(0); // count == 0, next will be error code
        data << uint8(0); // "Vendor has no inventory"
        session->SendPacket(&data);
        return;
    }

    uint8 itemCount = items->GetItemCount();
    uint8 count = 0;

    WorldPacket data(SMSG_LIST_INVENTORY, 8 + 1 + itemCount * 8 * 4);
    data << vendorGuid;

    size_t countPos = data.wpos();
    data << uint8(count);

    for (uint8 slot = 0; slot < itemCount; ++slot) {
        if (VendorItem const *item = items->GetItem(slot)) {
            if (ItemTemplate const *itemTemplate =
                    sObjectMgr->GetItemTemplate(item->item)) {
                if (!(itemTemplate->AllowableClass &
                        session->GetPlayer()->getClassMask()) &&
                        itemTemplate->Bonding == BIND_WHEN_PICKED_UP &&
                        !session->GetPlayer()->IsGameMaster())
                    continue;
                // Only display items in vendor lists for the team the
                // player is on. If GM on, display all items.
                if (!session->GetPlayer()->IsGameMaster() &&
                        ((itemTemplate->Flags2 & ITEM_FLAG2_FACTION_HORDE &&
                        session->GetPlayer()->GetTeamId() == TEAM_ALLIANCE) ||
                        (itemTemplate->Flags2 & ITEM_FLAG2_FACTION_ALLIANCE &&
                        session->GetPlayer()->GetTeamId() == TEAM_HORDE)))
                    continue;

                uint32 leftInStock = 0xFFFFFFFF;

                std::vector<SmartStonePlayerCommand> & playerCommands = sAZTH->GetAZTHPlayer(session->GetPlayer())->getSmartStoneCommands();
                int n = playerCommands.size();
                SmartStoneCommand command = sSmartStone->getCommandByItem(item->item);

                // we hide commands that the player already has
                for (int i = 0; i < n; i++) {
                    // sLog->outError("Smartstone: isnullcommand: %u, command: %u,
                    // playercommand: %u", isNullCommand(command), command.id,
                    // playerCommands[i]);

                    if (!isNullCommand(command) && command.id == playerCommands[i].id)
                        leftInStock = 0;
                }

                /* if (!session->GetPlayer()->IsGameMaster() && !leftInStock)
                     continue;*/

                /*ConditionList conditions =
                sConditionMgr->GetConditionsForNpcVendorEvent(SMARTSTONE_VENDOR_ENTRY,
                item->item);
                if (!sConditionMgr->IsObjectMeetToConditions(session->GetPlayer(),
                vendor, conditions))
                {
                    sLog->outError("SendListInventory: conditions not met for creature
                entry %u item %u", vendor->GetEntry(), item->item);
                    continue;
                }*/

                // reputation discount
                int32 price = item->IsGoldRequired(itemTemplate)
                        ? uint32(floor(itemTemplate->BuyPrice))
                        : 0;

                data << uint32(slot + 1); // client expects counting to start at 1
                data << uint32(item->item);
                data << uint32(itemTemplate->DisplayInfoID);
                data << int32(leftInStock);
                data << uint32(price);
                data << uint32(itemTemplate->MaxDurability);
                data << uint32(itemTemplate->BuyCount);
                data << uint32(item->ExtendedCost);

                if (++count >= MAX_VENDOR_ITEMS)
                    break;
            }
        }
    }

    if (count == 0) {
        data << uint8(0);
        session->SendPacket(&data);
        return;
    }

    data.put<uint8>(countPos, count);
    session->SendPacket(&data);
}

class azth_premium_rested_xp : public PlayerScript
{
public:
    azth_premium_rested_xp() : PlayerScript("azth_premium_rested_xp") { }

    void OnPlayerLogin(Player* player) override
    {
        if (!IsPremiumSmartStoneAccount(player) || !IsPremiumRestedXpEnabled(player))
            return;

        premiumRestedXpCharacters.insert(player->GetGUID().GetCounter());
        RefillPremiumRestedXp(player);
    }

    void OnPlayerGiveXP(Player* player, uint32& /*amount*/, Unit* /*victim*/, uint8 /*xpSource*/) override
    {
        if (premiumRestedXpCharacters.contains(player->GetGUID().GetCounter()))
            RefillPremiumRestedXp(player);
    }

    void OnPlayerLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        if (premiumRestedXpCharacters.contains(player->GetGUID().GetCounter()))
            RefillPremiumRestedXp(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        premiumRestedXpCharacters.erase(player->GetGUID().GetCounter());
    }
};

class aethro_lucky_angler : public PlayerScript
{
public:
    aethro_lucky_angler() : PlayerScript("aethro_lucky_angler") { }

    bool OnPlayerUpdateFishingSkill(Player* player, int32 /*skill*/, int32 /*zoneSkill*/, int32 /*chance*/,
                                    int32 /*roll*/) override
    {
        Item* mainHand = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        if (mainHand && mainHand->GetEntry() == AETHRO_LUCKY_ANGLER_ITEM && roll_chance_i(25))
            player->UpdateFishingSkill();

        return true;
    }

    void OnPlayerBeforeSendLoot(Player* player, ObjectGuid lootGuid, Loot* loot) override
    {
        Item* mainHand = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        if (!mainHand || mainHand->GetEntry() != AETHRO_LUCKY_ANGLER_ITEM ||
            loot->loot_type != LOOT_FISHING || !roll_chance_i(10))
            return;

        GameObject* bobber = player->GetMap()->GetGameObject(lootGuid);
        if (!bobber || bobber->GetGoType() != GAMEOBJECT_TYPE_FISHINGNODE)
            return;

        uint32 zone;
        uint32 area;
        bobber->GetZoneAndAreaId(zone, area);

        for (uint32 lootZone : {area, zone, 1u})
        {
            LootTemplate const* lootTemplate = LootTemplates_Fishing.GetLootFor(lootZone);
            if (!lootTemplate)
                continue;

            // A bonus roll can never provide quest-only fishing items.
            auto const questItemCount = loot->quest_items.size();
            lootTemplate->Process(*loot, LootTemplates_Fishing, LOOT_MODE_DEFAULT, player, 0, true);
            loot->quest_items.resize(questItemCount);
            return;
        }
    }
};

void AddSC_azth_smart_stone() // Add to scriptloader normally
{
    new azth_smart_stone();
    new azth_smartstone_world();
    new azth_smartstone_player_commands();
    new smartstone_vendor();
    new azth_premium_rested_xp();
    new aethro_lucky_angler();
}
