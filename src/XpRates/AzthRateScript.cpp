#include "ScriptMgr.h"
#include "Chat.h"
#include "Language.h"
#include "CustomRates.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "AZTH.h"

namespace
{
enum ExperienceRegistrarActions : uint32
{
    EXPERIENCE_RATE_01 = 1001,
    EXPERIENCE_RATE_05 = 1002,
    EXPERIENCE_RATE_1 = 1003,
    EXPERIENCE_RATE_2 = 1004,
    EXPERIENCE_RATE_3 = 1005,
    EXPERIENCE_RATE_4 = 1006,
    EXPERIENCE_RATE_5 = 1007,
};

float GetExperienceRegistrarRate(uint32 action)
{
    switch (action)
    {
        case EXPERIENCE_RATE_01: return 0.1f;
        case EXPERIENCE_RATE_05: return 0.5f;
        case EXPERIENCE_RATE_1: return 1.0f;
        case EXPERIENCE_RATE_2: return 2.0f;
        case EXPERIENCE_RATE_3: return 3.0f;
        case EXPERIENCE_RATE_4: return 4.0f;
        case EXPERIENCE_RATE_5: return 5.0f;
        default: return -1.0f;
    }
}

class aethro_experience_registrar : public CreatureScript
{
public:
    aethro_experience_registrar() : CreatureScript("aethro_experience_registrar") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        AddGossipItemFor(player, 0, "Set my experience rate to 0.1x", GOSSIP_SENDER_MAIN, EXPERIENCE_RATE_01);
        AddGossipItemFor(player, 0, "Set my experience rate to 0.5x", GOSSIP_SENDER_MAIN, EXPERIENCE_RATE_05);
        AddGossipItemFor(player, 0, "Set my experience rate to 1x", GOSSIP_SENDER_MAIN, EXPERIENCE_RATE_1);
        AddGossipItemFor(player, 0, "Set my experience rate to 2x", GOSSIP_SENDER_MAIN, EXPERIENCE_RATE_2);
        AddGossipItemFor(player, 0, "Set my experience rate to 3x", GOSSIP_SENDER_MAIN, EXPERIENCE_RATE_3);
        AddGossipItemFor(player, 0, "Set my experience rate to 4x", GOSSIP_SENDER_MAIN, EXPERIENCE_RATE_4);
        AddGossipItemFor(player, 0, "Set my experience rate to 5x", GOSSIP_SENDER_MAIN, EXPERIENCE_RATE_5);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);
        float rate = GetExperienceRegistrarRate(action);
        if (rate >= 0.0f)
            sAZTH->GetAZTHPlayer(player)->AzthSelfChangeXp(rate);

        CloseGossipMenuFor(player);
        return true;
    }
};

}

class AzthXPRatePlayerScripts : public PlayerScript
{
public:
    AzthXPRatePlayerScripts() : PlayerScript("AzthXPRatePlayerScripts") { }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        // Quests use Player::GetQuestRate and dungeon kills are adjusted by the existing
        // KillRewarder hook. Apply the selected personal rate only to ordinary PvE kills.
        if (!player || !victim || xpSource != XPSOURCE_KILL || victim->ToPlayer() ||
            victim->GetMap()->IsDungeon() || victim->GetMap()->IsBattlegroundOrArena())
            return;

        amount = uint32(float(amount) * sAZTH->GetAZTHPlayer(player)->GetPlayerQuestRate());
    }

    void OnPlayerDelete(ObjectGuid guid, uint32 /*AccountID*/) override
    {
        CustomRates::DeleteRateFromDB(guid);
    }

    void OnPlayerLogin(Player* player) override
    {
        float rate = CustomRates::GetXpRateFromDB(player);

        // player has custom xp rate set. Load it from DB. Otherwise use default set in AzthPlayer::AzthPlayer
        if (rate != -1)
        {
            sAZTH->GetAZTHPlayer(player)->SetPlayerQuestRate(rate);

            if (sAZTH->IsCustomXPShowOnLogin())
            {
                if (!rate)
                    ChatHandler(player->GetSession()).SendSysMessage("|CFF7BBEF7[Custom Rates]|r: Your quest XP rate was set to 0. You won't gain any XP from quest completation.");
                else
                    ChatHandler(player->GetSession()).PSendSysMessage("|CFF7BBEF7[Custom Rates]|r: Your quest XP rate was set to %.2f.", rate);
            }
        }
    }
};

void AddSC_Custom_Rates()
{
    new AzthXPRatePlayerScripts();
    new aethro_experience_registrar();
}
