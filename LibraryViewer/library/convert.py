import xmltodict
import json


def flatten_dict(d):

    out = []
    for song in d:
        newSong = {}

        strCount = 0
        intCount = 0
        dateCount = 0

        def getStr(id):
            nonlocal strCount
            if id not in song["key"]:
                return "undef"
            if len(song["string"]) <= strCount:
                return "error"
            strCount += 1
            return song["string"][strCount - 1]
        
        def getInt(id):
            nonlocal intCount
            if id not in song["key"]:
                return -1
            if len(song["integer"]) <= intCount:
                return -1
            intCount += 1
            return song["integer"][intCount - 1]
        
        def getDate(id):
            nonlocal dateCount
            if id not in song["key"]:
                return "undef"
            if len(song["date"]) <= dateCount:
                return "error"
            dateCount += 1
            return song["date"][dateCount - 1]
        
        newSong["Track ID"] = getInt("Track ID")
        newSong["Name"] = getStr("Name")
        newSong["Artist"] = getStr("Artist")
        newSong["Album Artist"] = getStr("Album Artist")
        newSong["Composer"] = getStr("Composer")
        newSong["Album"] = getStr("Album")
        newSong["Genre"] = getStr("Genre")
        newSong["Kind"] = getStr("Kind")
        newSong["Size"] = getInt("Size")
        newSong["Total Time"] = getInt("Total Time")
        newSong["Disc Number"] = getInt("Disc Number")
        newSong["Disc Count"] = getInt("Disc Count")
        newSong["Track Number"] = getInt("Track Number")
        newSong["Track Count"] = getInt("Track Count")
        newSong["Year"] = getInt("Year")
        newSong["Date Modified"] = getDate("Date Modified")
        newSong["Date Added"] = getDate("Date Added")
        newSong["Bit Rate"] = getInt("Bit Rate")
        newSong["Sample Rate"] = getInt("Sample Rate")
        newSong["Play Count"] = getInt("Play Count")
        newSong["Play Date"] = getInt("Play Date")
        newSong["Play Date UTC"] = getDate("Play Date UTC")
        newSong["Skip Count"] = getInt("Skip Count")
        newSong["Skip Date"] = getDate("Skip Date")
        newSong["Release Date"] = getDate("Release Date")
        newSong["Album Rating"] = getInt("Album Rating")
        newSong["Album Rating Computed"] = "Album Rating Computed" in song["key"]
        newSong["Loved"] = "Loved" in song["key"]
        newSong["Album Loved"] = "Album Loved" in song["key"]
        newSong["Explicit"] = "Explicit" in song["key"]
        newSong["Compilation"] = "Compilation" in song["key"]
        newSong["Artwork Count"] = getInt("Artwork Count")
        newSong["Sort Album"] = getStr("Sort Album")
        newSong["Sort Artist"] = getStr("Sort Artist")
        newSong["Sort Name"] = getStr("Sort Name")
        newSong["Persistent ID"] = getStr("Persistent ID")
        newSong["Track Type"] = getStr("Track Type")

        out.append(newSong)

    return out

with open('Library.xml', 'r', encoding='utf-8') as xml_file:
    data_dict = xmltodict.parse(xml_file.read())

# Extract the "Tracks" dictionary to be flattened
tracks_dict = data_dict['plist']['dict']['dict']

flat_tracks_dict = flatten_dict(tracks_dict["dict"])


json_data = json.dumps(flat_tracks_dict, indent=2)

with open('output.json', 'w', encoding='utf-8') as json_file:
    json_file.write(json_data)
