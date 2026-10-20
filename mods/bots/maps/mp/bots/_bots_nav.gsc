// Navigation for the simple bots: one waypoint graph per map, built with the
// libcod graph* functions and kept in waypoints/<map>_v2.graph, and A* paths
// over it. Without a graph the bots walk straight at their goal.

init()
{
	level.botGraph = undefined;

	file = "waypoints/" + getCvar("mapname") + "_v2.graph";
	graph = graphLoad(file);

	if(isDefined(graph) && graphGetNodeCount(graph) > 0)
	{
		level.botGraph = graph;
		debugLog("BOTNAV graph loaded " + graphGetNodeCount(graph));
		return;
	}

	if(isDefined(graph))
		graphRemove(graph);

	graph = generateGraph();
	if(!isDefined(graph))
	{
		debugLog("BOTNAV graph none");
		return;
	}

	level.botGraph = graph;
	graphSave(graph, file);
	debugLog("BOTNAV graph generated " + graphGetNodeCount(graph));
}

// Every spawn point of every gametype seeds the flood fill, so floors and
// areas a single seed would miss are covered too. They go in one call so the
// flood grows from all of them at once: one call per spawn let the first fill
// the whole node budget.
generateGraph()
{
	graph = graphCreate();
	if(!isDefined(graph))
		return undefined;

	seeds = [];
	classes = spawnClasses();
	for(c = 0; c < classes.size; c++)
	{
		spawns = getentarray(classes[c], "classname");
		for(i = 0; i < spawns.size; i++)
			seeds[seeds.size] = spawns[i].origin;
	}

	graphAutodiscoverEx(graph, seeds, 64);

	if(graphGetNodeCount(graph) == 0)
	{
		graphRemove(graph);
		return undefined;
	}

	return graph;
}

spawnClasses()
{
	classes = [];
	classes[0] = "mp_dm_spawn";
	classes[1] = "mp_tdm_spawn";
	classes[2] = "mp_sd_spawn_attacker";
	classes[3] = "mp_sd_spawn_defender";
	classes[4] = "mp_ctf_spawn_allied";
	classes[5] = "mp_ctf_spawn_axis";
	return classes;
}

// Clean navigation state for a new life.
resetNav()
{
	self.botPath = undefined;
	self.botPathIndex = 0;
	self.botPathGoal = -1;
	self.botRepathAt = 0;
	self.botBlocked = [];
	self.botBlockedUntil = 0;
	self.botStuckCount = 0;
	self.botDirectUntil = 0;
	self.botNavDirect = true;
	self.botBestDist = 999999;
	self.botProgressAt = 0;
	self.botHintPos = undefined;
	self.botHintTime = 0;
	self.botLastSeenPos = undefined;
	self.botLastSeenTime = 0;
	self.botPatrolPos = undefined;
	self.botPatrolUntil = 0;
	self.botGoalKind = "";
	self.botLastContact = getTime();
}

// Yaw to walk towards goalPos: along the graph when there is one (and then
// self.botNavDirect is false), otherwise straight at the goal.
navYaw(goalPos)
{
	now = getTime();
	self.botNavDirect = true;

	if(!isDefined(level.botGraph) || now < self.botDirectUntil)
		return self yawTo(goalPos);

	if(now >= self.botBlockedUntil)
		self.botBlocked = [];

	goalNode = graphFindClosestNode(level.botGraph, goalPos);
	if(!isDefined(goalNode))
		return self yawTo(goalPos);

	if(!isDefined(self.botPath) || now >= self.botRepathAt || goalNode != self.botPathGoal)
		self repath(goalNode, now);

	// no path, or the end of a partial one: straight on from here
	if(!isDefined(self.botPath) || self.botPathIndex >= self.botPath.size)
		return self yawTo(goalPos);

	nodePos = graphGetNodeOrigin(level.botGraph, self.botPath[self.botPathIndex]);

	// at the node, or already closer to the next one (cutting corners)
	reached = flatDist(self.origin, nodePos) < 32;
	if(!reached && self.botPathIndex + 1 < self.botPath.size)
	{
		nextPos = graphGetNodeOrigin(level.botGraph, self.botPath[self.botPathIndex + 1]);
		reached = flatDist(self.origin, nextPos) < flatDist(nodePos, nextPos);
	}

	if(reached)
	{
		debugLog("BOTNAV " + self getEntityNumber() + " node " + self.botPath[self.botPathIndex]);
		self.botPathIndex++;
		self.botStuckCount = 0;
		self startProgress(now);

		if(self.botPathIndex >= self.botPath.size)
			return self yawTo(goalPos);

		nodePos = graphGetNodeOrigin(level.botGraph, self.botPath[self.botPathIndex]);
	}

	dist = flatDist(self.origin, nodePos);
	if(dist < self.botBestDist - 16)
	{
		self.botBestDist = dist;
		self.botProgressAt = now + 1500;
	}
	else if(now >= self.botProgressAt)
	{
		self stuckOn(self.botPath[self.botPathIndex], now);
		return self yawTo(nodePos);
	}

	self.botNavDirect = false;
	return self yawTo(nodePos);
}

repath(goalNode, now)
{
	self.botPathGoal = goalNode;
	self.botRepathAt = now + 3000;
	self.botPathIndex = 0;
	self.botPath = undefined;
	self startProgress(now);

	start = graphFindClosestNode(level.botGraph, self.origin);
	if(!isDefined(start))
		return;

	debugLog("BOTNAV " + self getEntityNumber() + " repath");

	// A* never skips its start node, so a path from a blocked node would lead
	// straight back to it: walk straight for a moment to get away from it
	for(i = 0; i < self.botBlocked.size; i++)
	{
		if(self.botBlocked[i] == start)
		{
			self.botDirectUntil = now + 3000;
			return;
		}
	}

	if(self.botBlocked.size)
		self.botPath = graphFindPath(level.botGraph, start, goalNode, self.botBlocked);
	else
		self.botPath = graphFindPath(level.botGraph, start, goalNode);
}

// No progress towards a node: avoid it for a while; after three in a row,
// walk straight for a moment.
stuckOn(node, now)
{
	debugLog("BOTNAV " + self getEntityNumber() + " stuck " + node);
	self.botStuckCount++;
	self.botBlocked[self.botBlocked.size] = node;
	self.botBlockedUntil = now + 10000;

	if(self.botStuckCount >= 3)
	{
		self.botStuckCount = 0;
		self.botDirectUntil = now + 3000;
		self.botPath = undefined;
		return;
	}

	self.botRepathAt = 0;
}

startProgress(now)
{
	self.botBestDist = 999999;
	self.botProgressAt = now + 1500;
}

yawTo(pos)
{
	angles = vectorToAngles(pos - self.origin);
	return angles[1];
}

flatDist(a, b)
{
	return distance((a[0], a[1], 0), (b[0], b[1], 0));
}

// Where to go with no enemy in sight: a recent hint, then a patrol point,
// and after 20 s without contact the nearest enemy wherever it is.
chooseGoal()
{
	now = getTime();

	objective = self maps\mp\bots\_bots_obj::objectiveGoal();
	if(isDefined(objective))
	{
		if(self.botGoalKind != "objective")
		{
			debugLog("BOTNAV " + self getEntityNumber() + " goal objective");
			self.botGoalKind = "objective";
		}
		return objective;
	}
	kind = "patrol";
	pos = undefined;

	if(isDefined(self.botHintPos) && now - self.botHintTime < 10000)
	{
		if(distanceSquared(self.origin, self.botHintPos) < 96 * 96)
			self.botHintPos = undefined; // nobody there any more
		else
		{
			kind = "hint";
			pos = self.botHintPos;
		}
	}

	if(!isDefined(pos) && now - self.botLastContact > 20000 && isDefined(self.botNearestEnemy))
	{
		kind = "enemy";
		pos = self.botNearestEnemy.origin;
	}

	if(!isDefined(pos))
	{
		if(isDefined(self.botPatrolPos) && distanceSquared(self.origin, self.botPatrolPos) < 96 * 96)
		{
			debugLog("BOTNAV " + self getEntityNumber() + " patrol reached");
			self.botPatrolPos = undefined;
		}

		// an unreachable patrol point is given up after a while
		if(!isDefined(self.botPatrolPos) || now >= self.botPatrolUntil)
		{
			self.botPatrolPos = self pickPatrolPos();
			self.botPatrolUntil = now + 30000;
		}

		pos = self.botPatrolPos;
	}

	if(kind != self.botGoalKind)
	{
		debugLog("BOTNAV " + self getEntityNumber() + " goal " + kind);
		self.botGoalKind = kind;
	}

	return pos;
}

// Half the time an enemy spawn point (SD and CTF have team spawns), otherwise
// a random far node.
pickPatrolPos()
{
	spawns = self enemySpawns();
	if(spawns.size && randomInt(2) == 0)
		return spawns[randomInt(spawns.size)].origin;

	if(isDefined(level.botGraph))
	{
		count = graphGetNodeCount(level.botGraph);
		pos = undefined;
		for(i = 0; i < 8; i++)
		{
			pos = graphGetNodeOrigin(level.botGraph, randomInt(count));
			if(distanceSquared(self.origin, pos) > 1000 * 1000)
				break;
		}
		return pos;
	}

	spawns = allSpawns();
	if(spawns.size)
		return spawns[randomInt(spawns.size)].origin;

	return undefined;
}

enemySpawns()
{
	none = [];
	if(!level.botsTeamBased)
		return none;

	enemy = "axis";
	if(self.pers["team"] == "axis")
		enemy = "allies";

	gametype = getCvar("g_gametype");
	if(gametype == "ctf")
	{
		if(enemy == "allies")
			return getentarray("mp_ctf_spawn_allied", "classname");
		return getentarray("mp_ctf_spawn_axis", "classname");
	}

	if(gametype == "sd")
	{
		if(isDefined(game["attackers"]) && enemy == game["attackers"])
			return getentarray("mp_sd_spawn_attacker", "classname");
		return getentarray("mp_sd_spawn_defender", "classname");
	}

	return none;
}

allSpawns()
{
	all = [];
	classes = spawnClasses();
	for(c = 0; c < classes.size; c++)
	{
		spawns = getentarray(classes[c], "classname");
		for(i = 0; i < spawns.size; i++)
			all[all.size] = spawns[i];
	}
	return all;
}

debugLog(text)
{
	// read each time, so it can be turned on in the middle of a map
	if(getCvarInt("bots_debug"))
		logprint(text + "\n");
}
