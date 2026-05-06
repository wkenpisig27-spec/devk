#include "Stdafx.h"
#include "PacketCmd.h"
#include "LicenseValidator.h"

const char* APPLICATION_ID = "1452585104688939018";
int SendPresence = 1;
time_t StartTime = 0;
static bool isDiscordInitialized = false;

void updateDiscordPresence(const char* details, const char* state) {
	if (!isDiscordInitialized || !SendPresence) {
		return;
	}
	
	// Removed invalid include from here

	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.state = state;
	discordPresence.details = details;
	discordPresence.startTimestamp = (int64_t)StartTime;
	discordPresence.largeImageKey = "spo_logo";

    // [License Integration] Use Server Name from License
    static std::string largeText = "Pirate King Online";
    static bool licChecked = false;
    if (!licChecked) {
        auto licInfo = License::GetCurrentLicenseInfo();
        if (licInfo.valid && !licInfo.owner.empty()) {
            largeText = licInfo.owner;
        }
        licChecked = true;
    }
	discordPresence.largeImageText = largeText.c_str();

	discordPresence.smallImageKey = "";
	discordPresence.smallImageText = "";
	discordPresence.instance = 1;
	Discord_UpdatePresence(&discordPresence);
}

void handleDiscordReady(const DiscordUser* connectedUser) {
	LG("Discord", "Discord: connected to user %s#%s - %s\n",
	   connectedUser->username,
	   connectedUser->discriminator,
	   connectedUser->userId);
}

void handleDiscordDisconnected(int errcode, const char* message) {
	LG("Discord", "Discord: disconnected (%d: %s)\n", errcode, message);
}

void handleDiscordError(int errcode, const char* message) {
	LG("Discord", "Discord: error (%d: %s)\n", errcode, message);
}

void discordInit() {
	if (isDiscordInitialized) {
		return; // Already initialized
	}
	
	StartTime = time(0);
	
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	handlers.ready = handleDiscordReady;
	handlers.disconnected = handleDiscordDisconnected;
	handlers.errored = handleDiscordError;
	
	Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
	isDiscordInitialized = true;
	
	// Set initial presence
	updateDiscordPresence("Starting Game...", "");
}

void discordShutdown() {
	if (!isDiscordInitialized) {
		return;
	}
	
	Discord_ClearPresence();
	Discord_Shutdown();
	isDiscordInitialized = false;
}

void discordUpdate() {
	if (isDiscordInitialized) {
		Discord_RunCallbacks();
	}
}

// Helper function for updating presence with party info
void updateDiscordPresenceWithParty(const char* details, const char* location, int partySize, int maxPartySize) {
	if (!isDiscordInitialized || !SendPresence) {
		return;
	}
	
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.state = location;
	discordPresence.details = details;
	discordPresence.startTimestamp = (int64_t)StartTime;
	discordPresence.largeImageKey = "spo_logo";
	
    // [License Integration] Use Server Name from License
    static std::string largeText = "Pirate King Online";
    static bool licChecked = false;
    if (!licChecked) {
        auto licInfo = License::GetCurrentLicenseInfo();
        if (licInfo.valid && !licInfo.owner.empty()) {
            largeText = licInfo.owner;
        }
        licChecked = true;
    }
    discordPresence.largeImageText = largeText.c_str();
	
	if (partySize > 1) {
		discordPresence.partySize = partySize;
		discordPresence.partyMax = maxPartySize;
		discordPresence.partyId = "party";
		discordPresence.smallImageKey = "party";
		discordPresence.smallImageText = "In a Party";
	}
	
	discordPresence.instance = 1;
	Discord_UpdatePresence(&discordPresence);
}
