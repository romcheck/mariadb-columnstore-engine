/*
 * MetadataFile.cpp
 */
#include "MetadataFile.h"
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#define max(x, y) (x > y ? x : y)
#define min(x, y) (x < y ? x : y)

namespace storagemanager
{

MetadataFile::MetadataFile()
{
    mpConfig = Config::get();
    mpLogger = SMLogging::get();
    mObjectSize = 5 * (1<<20);
    try 
    {
        mObjectSize = stoul(mpConfig->getValue("ObjectStorage", "object_size"));
    }
    catch (...)
    {
        cerr << "ObjectStorage/object_size must be set to a numeric value" << endl;
        throw;
    }
    mVersion=1;
    mRevision=1;
}


MetadataFile::MetadataFile(const char* filename)
{
    mpConfig = Config::get();
    mpLogger = SMLogging::get();
    mObjectSize = 5 * (1<<20);
    try
    {
        mObjectSize = stoul(mpConfig->getValue("ObjectStorage", "object_size"));
    }
    catch (...)
    {
        cerr << "ObjectStorage/object_size must be set to a numeric value" << endl;
        throw;
    }
    string metadataFilename = string(filename) + ".meta";
    if (boost::filesystem::exists(metadataFilename))
    {
        boost::property_tree::ptree jsontree;
        boost::property_tree::read_json(metadataFilename, jsontree);
        metadataObject newObject;
        mVersion = jsontree.get<int>("version");
        mRevision = jsontree.get<int>("revision");

        BOOST_FOREACH(const boost::property_tree::ptree::value_type &v, jsontree.get_child("objects"))
        {
            metadataObject newObject;
            newObject.offset = v.second.get<uint64_t>("offset");
            newObject.length = v.second.get<uint64_t>("length");
            newObject.name = v.second.get<string>("name");
            mObjects.push_back(newObject);
        }
    }
    else
    {
        mVersion = 1;
        mRevision = 1;
        updateMetadata(filename);
    }
}

MetadataFile::~MetadataFile()
{

}

vector<metadataObject> MetadataFile::metadataRead(off_t offset, size_t length)
{
    vector<metadataObject> returnObjs;
    uint64_t startData = offset;
    uint64_t endData = offset + length;
    uint64_t dataRemaining = length;
    bool foundStart = false;
    for (std::vector<metadataObject>::iterator i = mObjects.begin(); i != mObjects.end(); ++i)
    {
        uint64_t startObject = i->offset;
        uint64_t endObject = i->offset + i->length;
        uint64_t maxEndObject = i->offset + mObjectSize;
        // This logic assumes objects are in ascending order of offsets
        if (startData >= startObject && (startData < endObject || startData < maxEndObject))
        {
            returnObjs.push_back(*i);
            if (startData >= endObject)
            {
                // data starts and the end of current object and can atleast partially fit here update length
                i->length += min((maxEndObject-startData),dataRemaining);
            }
            foundStart = true;
        }
        else if (endData >= startObject && (endData < endObject || endData < maxEndObject))
        {
            // data ends in this object
            returnObjs.push_back(*i);
            if (endData >= endObject)
            {
                // data end is beyond old length
                i->length += (endData - endObject);
            }
        }
        else if (endData >= startObject && foundStart)
        {
            // data overlaps this object
            returnObjs.push_back(*i);
        }
    }

    return returnObjs;
}

metadataObject MetadataFile::addMetadataObject(const char *filename, size_t length)
{
    metadataObject addObject,lastObject;
    if (!mObjects.empty())
    {
        metadataObject lastObject = mObjects.back();
        addObject.offset = lastObject.offset + lastObject.length;
    }
    else
    {
        addObject.offset = 0;
    }
    addObject.length = length;
    string newObjectKey = getNewKey(filename, addObject.offset, addObject.length);
    addObject.name = string(newObjectKey);
    mObjects.push_back(addObject);

    return addObject;
}


int MetadataFile::updateMetadata(const char *filename)
{
    string metadataFilename = string(filename) + ".meta";
    boost::property_tree::ptree jsontree;
    boost::property_tree::ptree objs;
    jsontree.put("version",mVersion);
    jsontree.put("revision",mRevision);
    for (std::vector<metadataObject>::const_iterator i = mObjects.begin(); i != mObjects.end(); ++i)
    {
        boost::property_tree::ptree object;
        object.put("offset",i->offset);
        object.put("length",i->length);
        object.put("name",i->name);
        objs.push_back(std::make_pair("", object));
    }
    jsontree.add_child("objects", objs);
    write_json(metadataFilename, jsontree);
}

string MetadataFile::getNewKeyFromOldKey(const string &oldKey)
{
    boost::uuids::uuid u;
    string ret(oldKey);
    strcpy(&ret[0], boost::uuids::to_string(u).c_str());
    return ret;
}

string MetadataFile::getNewKey(string sourceName, size_t offset, size_t length)
{
    boost::uuids::uuid u;
    stringstream ss;

    for (int i = 0; i < sourceName.length(); i++)
    {
        if (sourceName[i] == '/')
        {
            sourceName[i] = '-';
        }
    }

    ss << u << "_" << offset << "_" << length << "_" << sourceName;
    return ss.str();
}

void MetadataFile::printObjects()
{
    printf("Version: %i Revision: %i\n",mVersion,mRevision);
    for (std::vector<metadataObject>::const_iterator i = mObjects.begin(); i != mObjects.end(); ++i)
    {
        printf("Name: %s Length: %lu Offset: %lu\n",i->name.c_str(),i->length,i->offset);
    }
}


}



