/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2011 Couchbase, Inc
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
#pragma once

#include "config.h"

#include "kvshard.h"
#include "utility.h"

#include <platform/non_negative_counter.h>

#include <vector>

class KVBucket;
class VBucket;

/**
 * A map of known vbuckets.
 */
class VBucketMap {

// TODO: Remove this once there's a cleaner API to iterator KVShards.
friend class EPBucket;
friend class KVBucket;
friend class Warmup;

    class VBucketConfigChangeListener : public ValueChangedListener {
    public:
        VBucketConfigChangeListener(VBucketMap& vbucketMap)
            : map(vbucketMap) {}

        void sizeValueChanged(const std::string &key, size_t value) override;

    private:
        VBucketMap& map;
    };

public:

    VBucketMap(Configuration& config, KVBucket& store);

    /**
     * Add the VBucket to the map - extending the lifetime of the object until
     * it is removed from the map via dropAndDeleteVBucket.
     * @param vb shared pointer to the VBucket we are storing.
     */
    ENGINE_ERROR_CODE addBucket(VBucketPtr vb);

    void enablePersistence(EPBucket& ep);

    /**
     * Drop the vbucket from the map and setup deferred deletion of the VBucket.
     * Once the VBucketPtr has no more references the vbucket is deleted, but
     * deletion occurs via a task that is scheduled by the VBucketPtr deleter,
     * ensuring no front-end thread deletes the memory/disk associated with the
     * VBucket.
     *
     * @param id The VB to drop
     * @param cookie Optional connection cookie, this cookie will be notified
     *        when the deletion task is completed.
     */
    void dropVBucketAndSetupDeferredDeletion(Vbid id, const void* cookie);
    VBucketPtr getBucket(Vbid id) const;

    // Returns the size of the map, i.e. the total number of VBuckets it can
    // contain.
    size_t getSize() const {
        return size;
    }
    std::vector<Vbid> getBuckets() const;
    std::vector<Vbid> getBucketsSortedByState() const;

    /**
     * Returns a vector containing the vbuckets from the map that are in the
     * given state.
     * @param state  the state used to filter which vbuckets to return
     * @return  vector of vbuckets that are in the given state.
     */
    std::vector<Vbid> getBucketsInState(vbucket_state_t state) const;

    std::vector<std::pair<Vbid, size_t>> getActiveVBucketsSortedByChkMgrMem()
            const;

    /**
     * Get the memory usage by checkpoints for all active vbuckets.
     * @return Total checkpoint memory usage
     */
    size_t getActiveVBucketsTotalCheckpointMemoryUsage() const;
    KVShard* getShardByVbId(Vbid id) const;
    KVShard* getShard(KVShard::id_type shardId) const;
    size_t getNumShards() const;
    void setHLCDriftAheadThreshold(std::chrono::microseconds threshold);
    void setHLCDriftBehindThreshold(std::chrono::microseconds threshold);

    /**
     * Decrement the vb count for the given state.
     * @param state  the state for which the vb count is to be decremented.
     */
    void decVBStateCount(vbucket_state_t state) {
        --vbStateCount[state - vbucket_state_active];
    }

    /**
     * Increment the vb count for the given state.
     * @param state the state for which the vb count is to be incremented.
     */
    void incVBStateCount(vbucket_state_t state) {
        ++vbStateCount[state - vbucket_state_active];
    }

    /**
     * Get the state count for the given state.
     * @param state  the state for which the vb count is to be retrieved.
     * @rturn  the current vb count in the given state.
     */
    uint16_t getVBStateCount(vbucket_state_t state) const {
        return vbStateCount[state - vbucket_state_active];
    }

    /**
     * Set the state of the vBucket and set VBucketMap invariants
     * @param vb pointer to the vBucket to change
     * @param newState desired state
     * @param an optional lock (using nullptr) for the vBucket. Will be
     *        acquired by this function if not supplied
     * @return the old state of the vBucket
     */
    vbucket_state_t setState(VBucketPtr vb,
                             vbucket_state_t newState,
                             WriterLockHolder* vbStateLock);

private:

    std::vector<std::unique_ptr<KVShard>> shards;

    const size_t size;

    /**
     * Count of how many vbuckets in vbMap are in each of the four valid
     * states of active, replica, pending or dead.
     */
    std::array<cb::NonNegativeCounter<uint16_t>, 4> vbStateCount{{0, 0, 0, 0}};

    DISALLOW_COPY_AND_ASSIGN(VBucketMap);
};
