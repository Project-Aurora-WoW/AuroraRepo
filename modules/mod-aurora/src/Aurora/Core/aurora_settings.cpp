// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_settings.cpp - runtime configuration read from worldserver.conf.
//
// The AURORA_*_ENABLED macros in aurora_config.h are compile-time defaults
// (1 = on). They are overridden here by the `Aurora.*` keys, so an operator can
// switch a subsystem off without a rebuild.
//
// Applied rules
//   Aurora.Enable                     master switch; 0 disables everything
//   Aurora.Campaign.01.Enabled        campaign #1 instance and its npcs
//   Aurora.Shop.Enabled               .aurora shop list / buy
//   Aurora.Voting.Enabled             .aurora vote list / top / cast
//   Aurora.Events.Enabled             event scheduler tick
//   Aurora.OnlineRewards.Enabled      playtime thresholds and the streak
//   Aurora.Localization.Default       ruRU / enUS, used when the client
//                                     locale is not available
//
// Unknown or missing keys fall back to the compile-time value, so an old
// worldserver.conf keeps working unchanged.
// ============================================================================

#include "Config.h"
#include "Log.h"

#include "aurora_shared.h"

namespace Aurora
{
    namespace
    {
        struct Settings
        {
            bool master        = AURORA_PROJECT_ENABLED != 0;
            bool core          = AURORA_CORE_ENABLED != 0;
            bool campaign01    = AURORA_CAMPAIGN_01_ENABLED != 0;
            bool shop          = AURORA_SHOP_ENABLED != 0;
            bool voting        = AURORA_VOTING_ENABLED != 0;
            bool events        = true;
            bool onlineRewards = true;
            std::string defaultLocale = "ruRU";
        };

        Settings g_settings;
        bool     g_loaded = false;

        bool ReadBool(char const* key, bool fallback)
        {
            return sConfigMgr->GetOption<bool>(key, fallback);
        }
    }

    void LoadSettings()
    {
        g_loaded = true;

        // The master switch is read first: with Aurora.Enable = 0 every
        // feature flag is forced off, whatever the individual keys say.
        g_settings.master = ReadBool("Aurora.Enable", AURORA_PROJECT_ENABLED != 0);
        g_settings.core   = ReadBool("Aurora.Core.Enabled", AURORA_CORE_ENABLED != 0);

        bool const on = g_settings.master && g_settings.core;

        g_settings.campaign01    = on && ReadBool("Aurora.Campaign.01.Enabled", AURORA_CAMPAIGN_01_ENABLED != 0);
        g_settings.shop          = on && ReadBool("Aurora.Shop.Enabled", AURORA_SHOP_ENABLED != 0);
        g_settings.voting        = on && ReadBool("Aurora.Voting.Enabled", AURORA_VOTING_ENABLED != 0);
        g_settings.events        = on && ReadBool("Aurora.Events.Enabled", true);
        g_settings.onlineRewards = on && ReadBool("Aurora.OnlineRewards.Enabled", true);

        g_settings.defaultLocale = sConfigMgr->GetOption<std::string>("Aurora.Localization.Default", "ruRU");

        LOG_INFO("module", "[Aurora] Settings: enable={}, campaign01={}, shop={}, voting={}, "
                          "events={}, onlineRewards={}, locale={}.",
                 g_settings.master, g_settings.campaign01, g_settings.shop, g_settings.voting,
                 g_settings.events, g_settings.onlineRewards, g_settings.defaultLocale);
    }

    namespace Settings_
    {
        Settings const& Get()
        {
            if (!g_loaded)
                LoadSettings();

            return g_settings;
        }
    }

    bool IsProjectEnabled()   { return Settings_::Get().master; }
    bool IsCampaignEnabled(uint32 campaignId)
    {
        // Only campaign #1 has scripted content; the others are DB rows that
        // are not implemented yet and must stay off regardless of the config.
        if (campaignId != 1)
            return false;

        Settings const& s = Settings_::Get();
        return s.master && s.core && s.campaign01;
    }

    bool IsShopEnabled()      { Settings const& s = Settings_::Get(); return s.master && s.core && s.shop; }
    bool IsVotingEnabled()    { Settings const& s = Settings_::Get(); return s.master && s.core && s.voting; }
    bool IsEventsEnabled()    { Settings const& s = Settings_::Get(); return s.master && s.core && s.events; }
    bool IsStreakEnabled()    { Settings const& s = Settings_::Get(); return s.master && s.core && s.onlineRewards; }

    std::string const& GetDefaultLocale()
    {
        return Settings_::Get().defaultLocale;
    }
}
