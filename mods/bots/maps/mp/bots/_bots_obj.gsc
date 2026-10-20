// Objectives for the simple bots: SD (plant, guard, defuse) and CTF (steal,
// capture, defend, return). Each life a bot draws a role; the role gives
// _bots_nav a goal and, on arrival, an action. Other gametypes are untouched.

init()
{
	level.botObjMode = undefined;
	level.botBombPrecomputed = false;
	gametype = getCvar("g_gametype");

	if(gametype == "sd" && isDefined(level.bombmode) && level.bombmode < 2)
	{
		level.botZones = activeZones();
		if(!level.botZones.size)
			return;
		level.botObjMode = "sd";
		maps\mp\bots\_bots_nav::debugLog("BOTOBJ zones " + level.botZones.size + " of " + getentarray("bombzone", "targetname").size);
		for(i = 0; i < level.botZones.size; i++)
			precomputeTo(level.botZones[i].origin);
	}
	else if(gametype == "ctf")
	{
		level.botFlags = [];
		level.botFlags["allies"] = getent("allied_flag", "targetname");
		level.botFlags["axis"] = getent("axis_flag", "targetname");
		if(!isDefined(level.botFlags["allies"]) || !isDefined(level.botFlags["axis"]))
			return;
		if(!isDefined(level.botFlags["allies"].home_origin) || !isDefined(level.botFlags["axis"].home_origin))
			return;
		level.botObjMode = "ctf";
		precomputeTo(level.botFlags["allies"].home_origin);
		precomputeTo(level.botFlags["axis"].home_origin);
	}
}

// The bomb sites SD uses in this bomb mode: like the game, only the ones
// marked for it. A map without the marks keeps all of them.
activeZones()
{
	zones = getentarray("bombzone", "targetname");
	active = [];
	for(i = 0; i < zones.size; i++)
	{
		if(level.bombmode == 0 && isDefined(zones[i].script_bombmode_original))
			active[active.size] = zones[i];
		else if(level.bombmode == 1 && isDefined(zones[i].script_bombmode_single))
			active[active.size] = zones[i];
	}

	if(!active.size)
		return zones;
	return active;
}

// Paths to a fixed objective are computed once; graphFindPath then follows
// them without searching.
precomputeTo(pos)
{
	if(!isDefined(level.botGraph))
		return;

	node = graphFindClosestNode(level.botGraph, pos);
	if(isDefined(node))
		graphPrecomputePathsToNode(level.botGraph, node);
}

// A role for a new life. A bot only gets to be a hunter when a teammate
// already plays the objective, so small teams always play it.
resetObjective()
{
	self.botRole = "hunter";
	self.botZone = undefined;
	self.botGuardPos = undefined;
	self.botGuardCenter = undefined;
	self.botGuardUntil = 0;
	self.botGuardYaw = 0;
	self.botGuarding = false;
	self.botCrouched = false;
	self.botObjAction = undefined;
	self.botObjStart = 0;
	self.botObjGoal = undefined;
	self.botObjGoalAt = -1;
	self.botHadFlag = false;
	self.botReturnPos = undefined;

	if(!isDefined(level.botObjMode))
		return;

	role = undefined;
	if(level.botObjMode == "sd")
	{
		if(self.pers["team"] == game["attackers"])
			role = "plant";
		else
			role = "guard";

		if(randomInt(3) == 0 && self teamHasRole(role))
			role = "hunter";
		else
			self.botZone = level.botZones[randomInt(level.botZones.size)];
	}
	else
	{
		// as in SD, somebody always goes for the enemy flag
		role = "steal";
		if(randomInt(2) == 0 && self teamHasRole("steal"))
			role = "defend";
	}

	self setRole(role);
}

setRole(role)
{
	if(self.botRole == role && role != "hunter")
		return;

	self.botRole = role;
	maps\mp\bots\_bots_nav::debugLog("BOTOBJ " + self getEntityNumber() + " role " + role);
}

teamHasRole(role)
{
	players = getentarray("player", "classname");
	for(i = 0; i < players.size; i++)
	{
		other = players[i];
		if(other == self || !isAlive(other) || other.sessionstate != "playing")
			continue;
		if(other.pers["team"] == self.pers["team"] && isDefined(other.botRole) && other.botRole == role)
			return true;
	}
	return false;
}

// The goal of this bot's objective role, or undefined to let _bots_nav pick
// hints, patrols and enemies.
objectiveGoal()
{
	// objectiveAction may already have asked this tick
	now = getTime();
	if(self.botObjGoalAt == now)
		return self.botObjGoal;
	self.botObjGoalAt = now;
	self.botObjGoal = self computeGoal();
	return self.botObjGoal;
}

computeGoal()
{
	self.botGuarding = false;

	if(!isDefined(level.botObjMode) || roundEnded())
		return undefined;

	if(level.botObjMode == "sd")
		return self sdGoal();

	return self ctfGoal();
}

sdGoal()
{
	if(level.bombplanted)
	{
		bomb = getent("bombtrigger", "targetname");
		if(!isDefined(bomb))
			return undefined;

		if(!level.botBombPrecomputed)
		{
			precomputeTo(bomb.origin);
			level.botBombPrecomputed = true;
		}

		if(self.pers["team"] != level.planting_team)
		{
			self setRole("defuse");
			return bomb.origin;
		}

		if(self.botRole == "plant" || self.botRole == "protect")
		{
			self setRole("protect");
			return self guardSpot(bomb.origin);
		}

		return undefined;
	}

	// SD has no respawn within a round: a hunter takes over when the last
	// planter or guard of its team is gone
	if(self.botRole == "hunter")
	{
		role = "guard";
		if(self.pers["team"] == game["attackers"])
			role = "plant";
		if(!self teamHasRole(role))
		{
			self setRole(role);
			self.botZone = level.botZones[randomInt(level.botZones.size)];
		}
	}

	if(!isDefined(self.botZone))
		return undefined;
	if(self.botRole == "plant")
		return self.botZone.origin;
	if(self.botRole == "guard")
		return self guardSpot(self.botZone.origin);

	return undefined;
}

// A spot near center to watch from: a graph node within 400 units, changed
// every 15-25 s, facing an enemy spawn point.
guardSpot(center)
{
	now = getTime();
	self.botGuarding = true;

	if(isDefined(self.botGuardPos) && now < self.botGuardUntil && isDefined(self.botGuardCenter)
		&& distanceSquared(self.botGuardCenter, center) < 1)
		return self.botGuardPos;

	self.botGuardCenter = center;
	self.botGuardUntil = now + 15000 + randomInt(10000);
	self.botGuardPos = center;

	if(isDefined(level.botGraph))
	{
		near = graphGetAllNodes(level.botGraph, center, 400 * 400);
		if(isDefined(near) && near.size)
			self.botGuardPos = graphGetNodeOrigin(level.botGraph, near[randomInt(near.size)]);
	}

	spawns = self maps\mp\bots\_bots_nav::enemySpawns();
	if(spawns.size)
	{
		angles = vectorToAngles(spawns[randomInt(spawns.size)].origin - self.botGuardPos);
		self.botGuardYaw = angles[1];
	}
	else
		self.botGuardYaw = randomInt(360);

	return self.botGuardPos;
}

// What to do this tick on arrival: hold use on the bomb site or the bomb, or
// stand watch. Returns true when the tick is handled.
objectiveAction()
{
	if(!isDefined(level.botObjMode) || roundEnded())
	{
		self stopUse();
		return false;
	}

	trigger = undefined;
	action = undefined;

	if(level.botObjMode == "sd")
	{
		if(self.botRole == "plant" && !level.bombplanted && isDefined(self.botZone) && self isTouching(self.botZone))
		{
			trigger = self.botZone;
			action = "plant";
		}
		else if(self.botRole == "defuse" && level.bombplanted)
		{
			bomb = getent("bombtrigger", "targetname");
			if(isDefined(bomb) && self isTouching(bomb))
			{
				trigger = bomb;
				action = "defuse";
			}
		}
	}

	// the game only checks the ground when planting starts; once it links the
	// player to the bomb site isOnGround() is false
	if(isDefined(trigger) && (isDefined(self.botObjAction) || self isOnGround()))
	{
		if(!isDefined(self.botObjAction))
		{
			self.botObjAction = action;
			self.botObjStart = getTime();
			maps\mp\bots\_bots_nav::debugLog("BOTOBJ " + self getEntityNumber() + " " + action + " start");
		}
		self setWalkDir("none");
		self useButton(1);
		return true;
	}

	self stopUse();

	// standing watch skips chooseGoal, so refresh the objective here: the guard
	// spot rotates and roles change (a planted bomb sends defenders to defuse)
	if(self.botGuarding)
		self objectiveGoal();

	if(self.botGuarding && isDefined(self.botGuardPos) && distanceSquared(self.origin, self.botGuardPos) < 64 * 64)
	{
		self setWalkDir("none");
		self maps\mp\bots\_bots::lookAt((0, self.botGuardYaw + randomInt(40) - 20, 0));
		if(randomInt(30) == 0)
		{
			self.botCrouched = !self.botCrouched;
			if(self.botCrouched)
				self setBotStance("crouch");
			else
				self setBotStance("stand");
		}
		return true;
	}

	if(self.botCrouched)
	{
		self.botCrouched = false;
		self setBotStance("stand");
	}

	return false;
}

// An enemy came into sight while holding use: past half the time, finish.
keepUsing()
{
	if(!isDefined(self.botObjAction) || roundEnded())
		return false;

	// planted: let go and fight
	if(self.botObjAction == "plant" && level.bombplanted)
	{
		self stopUse();
		return false;
	}

	if(self.botObjAction == "plant")
		half = level.planttime * 500;
	else
		half = level.defusetime * 500;

	if(getTime() - self.botObjStart < half)
	{
		self stopUse();
		return false;
	}

	self setWalkDir("none");
	self useButton(1);
	return true;
}

stopUse()
{
	if(!isDefined(self.botObjAction))
		return;

	self useButton(0);

	done = false;
	if(self.botObjAction == "plant")
		done = level.bombplanted;
	else
		done = !isDefined(getent("bombtrigger", "targetname")) || roundEnded();

	if(done)
		maps\mp\bots\_bots_nav::debugLog("BOTOBJ " + self getEntityNumber() + " " + self.botObjAction + " done");

	self.botObjAction = undefined;
}

roundEnded()
{
	return isDefined(level.roundended) && level.roundended;
}

ctfGoal()
{
	team = self.pers["team"];
	own = level.botFlags[team];
	theirs = level.botFlags[otherTeam(team)];
	if(!isDefined(own) || !isDefined(theirs))
		return undefined;

	num = self getEntityNumber();

	// carrying the enemy flag: home, or wait near it until ours is back
	if(isDefined(self.flag))
	{
		if(!self.botHadFlag)
		{
			maps\mp\bots\_bots_nav::debugLog("BOTOBJ " + num + " pickup");
			self.botHadFlag = true;
		}
		self setRole("capture");
		if(own.atbase)
			return own.home_origin;
		return self guardSpot(own.home_origin);
	}

	// still alive and no longer carrying it: captured
	if(self.botHadFlag)
	{
		maps\mp\bots\_bots_nav::debugLog("BOTOBJ " + num + " capture");
		self.botHadFlag = false;
		self setRole("steal");
	}

	if(self.botRole == "steal")
	{
		carrier = flagCarrier(theirs);
		if(isDefined(carrier))
			return carrier.origin; // escort the teammate who has it
		return theirs.origin;
	}

	// defend
	if(own.atbase)
	{
		if(isDefined(self.botReturnPos) && distanceSquared(self.origin, self.botReturnPos) < 150 * 150)
			maps\mp\bots\_bots_nav::debugLog("BOTOBJ " + num + " return");
		self.botReturnPos = undefined;
		return self guardSpot(own.home_origin);
	}

	carrier = flagCarrier(own);
	if(isDefined(carrier))
	{
		self.botReturnPos = undefined;
		return carrier.origin;
	}

	// ours lies on the ground: touching it sends it home
	self.botReturnPos = own.origin;
	return own.origin;
}

flagCarrier(flag)
{
	players = getentarray("player", "classname");
	for(i = 0; i < players.size; i++)
	{
		if(isAlive(players[i]) && isDefined(players[i].flag) && players[i].flag == flag)
			return players[i];
	}
	return undefined;
}

otherTeam(team)
{
	if(team == "axis")
		return "allies";
	return "axis";
}
