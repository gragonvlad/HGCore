/*
 * Copyright (C) 2005-2008 MaNGOS <http://www.mangosproject.org/>
 *
 * Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef HELLGROUND_GRIDNOTIFIERSIMPL_H
#define HELLGROUND_GRIDNOTIFIERSIMPL_H

#include "GridNotifiers.h"
#include "WorldPacket.h"
#include "Corpse.h"
#include "Player.h"
#include "UpdateData.h"
#include "CreatureAI.h"
#include "SpellAuras.h"

template<class T>
inline void Hellground::VisibleNotifier::Visit(GridRefManager<T> &m)
{
    for(typename GridRefManager<T>::iterator iter = m.begin(); iter != m.end(); ++iter)
    {
        vis_guids.erase(iter->getSource()->GetGUID());
        _camera.UpdateVisibilityOf(iter->getSource(), i_data, i_visibleNow);
    }
}

inline void PlayerCreatureRelocationWorker(Player* p, Creature* c)
{
    if (!p->isAlive() || !c->isAlive())
        return;
    
    if (p->IsTaxiFlying() && !c->CanReactToPlayerOnTaxi())
        return;

    if (c->hasUnitState(UNIT_STAT_LOST_CONTROL | UNIT_STAT_SIGHTLESS | UNIT_STAT_IGNORE_ATTACKERS))
        return;

    // Creature AI reaction
    if (c->HasReactState(REACT_AGGRESSIVE) && !c->IsInEvadeMode() && c->IsAIEnabled)
        c->AI()->MoveInLineOfSight_Safe(p);
}

inline void CreatureCreatureRelocationWorker(Creature* c1, Creature* c2)
{
    if (c1->hasUnitState(UNIT_STAT_LOST_CONTROL | UNIT_STAT_SIGHTLESS | UNIT_STAT_IGNORE_ATTACKERS))
        return;

    if (c2->hasUnitState(UNIT_STAT_IGNORE_ATTACKERS))
        return;

    // Creature AI reaction
    if (c1->HasReactState(REACT_AGGRESSIVE) && !c1->IsInEvadeMode() && c1->IsAIEnabled)
        c1->AI()->MoveInLineOfSight_Safe(c2);
}

inline void Hellground::PlayerRelocationNotifier::Visit(CameraMapType &m)
{
    for(CameraMapType::iterator iter = m.begin(); iter != m.end(); ++iter)
    {
        iter->getSource()->UpdateVisibilityOf(&_player);

        // need to choose this or below one (this should be faster :P
        _player.GetCamera().UpdateVisibilityOf(iter->getSource()->GetBody());
    }

    //_player.GetCamera().UpdateVisibilityForOwner();
}

inline void Hellground::PlayerRelocationNotifier::Visit(CreatureMapType &m)
{
    for(CreatureMapType::iterator iter = m.begin(); iter != m.end(); ++iter)
        PlayerCreatureRelocationWorker(&_player, iter->getSource());
}

inline void Hellground::CreatureRelocationNotifier::Visit(PlayerMapType &m)
{
    if (!_creature.isAlive())
        return;

    for (PlayerMapType::iterator iter = m.begin(); iter != m.end(); ++iter)
        PlayerCreatureRelocationWorker(iter->getSource(), &_creature);
}

template<>
inline void Hellground::CreatureRelocationNotifier::Visit(CreatureMapType &m)
{
    if (!_creature.isAlive())
        return;

    for (CreatureMapType::iterator iter = m.begin(); iter != m.end(); ++iter)
    {
        CreatureCreatureRelocationWorker(iter->getSource(), &_creature);
        CreatureCreatureRelocationWorker(&_creature, iter->getSource());
    }
}

inline void Hellground::ObjectUpdater::Visit(CreatureMapType &m)
{
    for (CreatureMapType::iterator iter=m.begin(); iter != m.end(); ++iter)
        if (iter->getSource()->IsInWorld() && !iter->getSource()->isSpiritService())
        {
            WorldObject::UpdateHelper helper(iter->getSource());
            helper.Update(i_timeDiff);
        }
}

template<class T, class Check>
void Hellground::ObjectSearcher<T, Check>::Visit(GridRefManager<T>& m)
{
    // already found
    if (_object)
        return;

    for (typename GridRefManager<T>::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (_check(itr->getSource()))
        {
            _object = itr->getSource();
            return;
        }
    }
}

template<class T, class Check>
void Hellground::ObjectLastSearcher<T, Check>::Visit(GridRefManager<T>& m)
{
    for (typename GridRefManager<T>::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (_check(itr->getSource()))
            _object = itr->getSource();
    }
}

template<class T, class Check>
void Hellground::ObjectListSearcher<T, Check>::Visit(GridRefManager<T>& m)
{
    for (typename GridRefManager<T>::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (_check(itr->getSource()))
            _objects.push_back(itr->getSource());
    }
}

// Unit searchers
template<class Check>
void Hellground::UnitSearcher<Check>::Visit(CreatureMapType &m)
{
    // already found
    if (i_object)
        return;

    for (CreatureMapType::iterator itr=m.begin(); itr != m.end(); ++itr)
    {
        if (i_check(itr->getSource()))
        {
            i_object = itr->getSource();
            return;
        }
    }
}

template<class Check>
void Hellground::UnitSearcher<Check>::Visit(PlayerMapType &m)
{
    // already found
    if (i_object)
        return;

    for (PlayerMapType::iterator itr=m.begin(); itr != m.end(); ++itr)
    {
        if (i_check(itr->getSource()))
        {
            i_object = itr->getSource();
            return;
        }
    }
}

template<class Check>
void Hellground::UnitLastSearcher<Check>::Visit(CreatureMapType &m)
{
    for (CreatureMapType::iterator itr=m.begin(); itr != m.end(); ++itr)
    {
        if (i_check(itr->getSource()))
            i_object = itr->getSource();
    }
}

template<class Check>
void Hellground::UnitLastSearcher<Check>::Visit(PlayerMapType &m)
{
    for (PlayerMapType::iterator itr=m.begin(); itr != m.end(); ++itr)
    {
        if (i_check(itr->getSource()))
            i_object = itr->getSource();
    }
}

template<class Check>
void Hellground::UnitListSearcher<Check>::Visit(PlayerMapType &m)
{
    for (PlayerMapType::iterator itr=m.begin(); itr != m.end(); ++itr)
        if (i_check(itr->getSource()))
            i_objects.push_back(itr->getSource());
}

template<class Check>
void Hellground::UnitListSearcher<Check>::Visit(CreatureMapType &m)
{
    for (CreatureMapType::iterator itr=m.begin(); itr != m.end(); ++itr)
        if (i_check(itr->getSource()))
            i_objects.push_back(itr->getSource());
}

template<class Builder>
void Hellground::LocalizedPacketDo<Builder>::operator()( Player* p )
{
    uint32 loc_idx = p->GetSession()->GetSessionDbLocaleIndex();
    uint32 cache_idx = loc_idx+1;
    WorldPacket* data;

    // create if not cached yet
    if(i_data_cache.size() < cache_idx+1 || !i_data_cache[cache_idx])
    {
        if(i_data_cache.size() < cache_idx+1)
            i_data_cache.resize(cache_idx+1);

        data = new WorldPacket(SMSG_MESSAGECHAT, 200);

        i_builder(*data,loc_idx);

        i_data_cache[cache_idx] = data;
    }
    else
        data = i_data_cache[cache_idx];

    p->BroadcastPacketToSelf(data);
}

#endif
