// Simple bots for CoD2rev-Plus dedicated servers.
//
// A bot walks the map over a waypoint graph (_bots_nav.gsc): with nobody in
// sight it follows what it last saw or heard shooting, patrols towards the
// enemy side, and after 20 s without contact heads for the nearest enemy. In a
// fight (_bots_combat.gsc) it aims down the sight at range, throws grenades at
// mid range and knifes up close. In SD and CTF (_bots_obj.gsc) each life a bot
// draws a role: plant, guard, defuse or protect the bomb, or steal, capture,
// defend or return flags; SD in dual bomb mode is played by hunters only.
// Movement and firing use the libcod bot and graph functions, so this only
// works on a server built with libcod.
//
// Server variables:
//   bots_count  how many bots to keep connected (default 0)
//   bots_skill  0 to 10: aim error and reaction time (default 5)
//   bots_debug  1 writes BOTNAV, BOTCOMBAT and BOTOBJ lines to games_mp.log
//               (default 0; can be changed in the middle of a map)
//
// Waypoints: the first time a map is played the bots build a navigation graph
// from its spawn points and save it as waypoints/<map>_v2.graph in the mod
// folder; later loads read that file. Delete it to have it built again.
//
// Installed by scripts/install_bots.py, which also hooks init() into the
// gametype start.

init()
{
	if(getCvar("bots_count") == "")
		setCvar("bots_count", "0");
	if(getCvar("bots_skill") == "")
		setCvar("bots_skill", "5");

	level.botsTeamBased = (getCvar("g_gametype") != "dm");

	maps\mp\bots\_bots_nav::init();
	maps\mp\bots\_bots_obj::init();
	level thread manageBots();
}

// Keeps the number of connected bots equal to bots_count.
manageBots()
{
	wait 3;

	for(;;)
	{
		wanted = getCvarInt("bots_count");
		maxBots = getCvarInt("sv_maxclients") - 1; // always leave a slot for a player
		if(wanted > maxBots)
			wanted = maxBots;

		players = getentarray("player", "classname");
		have = 0;
		lastBot = undefined;

		for(i = 0; i < players.size; i++)
		{
			if(!(players[i] isBot()))
				continue;

			have++;
			lastBot = players[i];

			// bots stay connected across map changes, so pick them up again
			if(!isDefined(players[i].botRunning))
				players[i] thread runBot();
		}

		if(have < wanted && players.size < maxBots + 1)
		{
			bot = addtestclient();
			if(isDefined(bot))
				bot thread runBot();
		}
		else if(have > wanted && isDefined(lastBot))
			kick(lastBot getEntityNumber());

		wait 1;
	}
}

runBot()
{
	self endon("disconnect");

	self.botRunning = true;
	self releaseButtons();

	while(!isDefined(self.pers["team"]))
		wait 0.05;

	for(;;)
	{
		if(self.pers["team"] != "allies" && self.pers["team"] != "axis")
		{
			self notify("menuresponse", game["menu_team"], "autoassign");
			wait 0.5;
		}
		else if(!isDefined(self.pers["weapon"]))
		{
			team = self.pers["team"];
			self notify("menuresponse", game["menu_weapon_" + team], pickWeapon(game[team]));
			wait 0.5;
		}
		else if(self.sessionstate == "playing")
		{
			self playLife();
			self releaseButtons();

			// the game waits for the use key before respawning; bots send the event themselves
			wait 2 + randomFloat(2);
			self notify("respawn");
		}

		wait 0.25;
	}
}

pickWeapon(nationality)
{
	weapons = [];

	switch(nationality)
	{
	case "american":
		weapons[0] = "thompson_mp";
		weapons[1] = "m1garand_mp";
		weapons[2] = "bar_mp";
		break;
	case "british":
		weapons[0] = "sten_mp";
		weapons[1] = "enfield_mp";
		weapons[2] = "bren_mp";
		break;
	case "russian":
		weapons[0] = "ppsh_mp";
		weapons[1] = "mosin_nagant_mp";
		weapons[2] = "PPS42_mp";
		break;
	default:
		weapons[0] = "mp40_mp";
		weapons[1] = "kar98k_mp";
		weapons[2] = "mp44_mp";
		break;
	}

	return weapons[randomInt(weapons.size)];
}

releaseButtons()
{
	self setWalkDir("none");
	self fireWeapon(0);
	self reloadWeapon(0);
	self setBotStance("stand");
	self meleeWeapon(0);
	self adsAim(0);
	self holdBreath(0);
	self throwNade(0);
	self useButton(0);
}

// One life: runs until the bot dies or stops playing.
playLife()
{
	self endon("disconnect");

	skill = getCvarInt("bots_skill");
	if(skill < 0)
		skill = 0;
	if(skill > 10)
		skill = 10;

	self maps\mp\bots\_bots_nav::resetNav();
	self maps\mp\bots\_bots_combat::resetCombat();
	self maps\mp\bots\_bots_obj::resetObjective();

	angles = self getPlayerAngles();
	yaw = angles[1];

	avoidYaw = yaw;
	avoidUntil = 0;
	checkPos = self.origin;
	checkAt = getTime() + 1000;
	jumped = false;

	for(;;)
	{
		wait 0.1;

		if(!isAlive(self) || self.sessionstate != "playing")
			return;

		now = getTime();

		if(jumped)
		{
			self setBotStance("stand");
			jumped = false;
		}

		self scanEnemies();

		// a grenade throw owns the view and the buttons until it is done
		if(self.botThrowing)
			continue;

		if(isDefined(self.botTarget))
		{
			// past half the plant or defuse, finishing beats fighting
			if(self maps\mp\bots\_bots_obj::keepUsing())
				continue;

			self maps\mp\bots\_bots_combat::fight(self.botTarget, skill);
			self.botPath = undefined;
			checkPos = self.origin;
			checkAt = now + 1000;
			continue;
		}

		// nothing in sight: hunt hints, patrol, or after a while the nearest enemy
		self.botLastTarget = undefined;

		// the enemy just ducked out of sight: a grenade where it was
		if(isDefined(self.botLastSeenPos) && now - self.botLastSeenTime < 2000)
		{
			if(self maps\mp\bots\_bots_combat::tryGrenade(self.botLastSeenPos, skill))
				continue;
		}

		self maps\mp\bots\_bots_combat::idleReload();

		// planting, defusing or standing watch
		if(self maps\mp\bots\_bots_obj::objectiveAction())
			continue;

		goalPos = self maps\mp\bots\_bots_nav::chooseGoal();
		if(isDefined(goalPos))
			yaw = self maps\mp\bots\_bots_nav::navYaw(goalPos);

		// the random turns are for walking straight; on the graph, stuck
		// handling picks another path instead
		if(now < avoidUntil && self.botNavDirect)
			yaw = avoidYaw;

		// is something right in front, at knee and chest height?
		forward = anglesToForward((0, yaw, 0));
		blocked = false;
		low = bulletTrace(self.origin + (0, 0, 20), self.origin + (0, 0, 20) + scaleVector(forward, 40), false, self);
		high = bulletTrace(self.origin + (0, 0, 50), self.origin + (0, 0, 50) + scaleVector(forward, 40), false, self);
		if(low["fraction"] < 1 || high["fraction"] < 1)
			blocked = true;

		// or has the bot barely moved in the last second?
		if(now >= checkAt)
		{
			if(distance(self.origin, checkPos) < 30)
				blocked = true;
			checkPos = self.origin;
			checkAt = now + 1000;
		}

		if(blocked && now >= avoidUntil)
		{
			if(low["fraction"] < 1 && high["fraction"] >= 1)
			{
				// low obstacle: try to jump it and keep going
				self setBotStance("jump");
				jumped = true;
				avoidYaw = yaw;
				avoidUntil = now + 400;
			}
			else if(self.botNavDirect)
			{
				if(randomInt(2) == 0)
					avoidYaw = yaw + 60 + randomInt(80);
				else
					avoidYaw = yaw - 60 - randomInt(80);
				avoidUntil = now + 700 + randomInt(900);
				yaw = avoidYaw;
			}
		}

		self lookAt((0, yaw, 0));
		self setWalkDir("none");
		self setWalkDir("forward");
	}
}

scaleVector(vec, scale)
{
	return (vec[0] * scale, vec[1] * scale, vec[2] * scale);
}

lookAt(angles)
{
	pitch = angles[0];
	if(pitch > 180)
		pitch = pitch - 360;

	self setPlayerAngles((pitch, angles[1], 0));
}

// Sets self.botTarget to the nearest enemy in sight and self.botNearestEnemy to the nearest enemy at
// all, and records what the bot sees or hears as hints.
scanEnemies()
{
	self.botTarget = undefined;
	self.botNearestEnemy = undefined;

	eye = self getEye();
	forward = anglesToForward(self getPlayerAngles());
	nearestSeen = 3000 * 3000;
	nearestAny = 999999999;

	players = getentarray("player", "classname");
	for(i = 0; i < players.size; i++)
	{
		other = players[i];

		if(other == self || !isAlive(other) || other.sessionstate != "playing")
			continue;
		if(level.botsTeamBased && other.pers["team"] == self.pers["team"])
			continue;

		distSq = distanceSquared(self.origin, other.origin);

		if(distSq < nearestAny)
		{
			nearestAny = distSq;
			self.botNearestEnemy = other;
		}

		// gunfire gives the shooter away, even out of sight
		if(distSq < 1500 * 1500 && other isFiring())
		{
			self.botHintPos = other.origin;
			self.botHintTime = getTime();
			self.botLastContact = getTime();
		}

		if(distSq >= nearestSeen)
			continue;

		// bots do not see behind them, unless the enemy is very close
		if(distSq > 250 * 250 && vectorDot(forward, vectorNormalize(other.origin - self.origin)) < 0.3)
			continue;

		if(sightTracePassed(eye, other getEye(), false, self))
		{
			nearestSeen = distSq;
			self.botTarget = other;
		}
	}

	if(isDefined(self.botTarget))
	{
		self.botHintPos = self.botTarget.origin;
		self.botHintTime = getTime();
		self.botLastSeenPos = self.botTarget.origin;
		self.botLastSeenTime = getTime();
		self.botLastContact = getTime();
	}
}
