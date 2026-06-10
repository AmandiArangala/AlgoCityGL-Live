import json
import urllib.request
import urllib.parse
from pathlib import Path
import socket

# Coordinates bounding box for Borella Junction (approx 1.8km x 1.9km)
# Center: Lat 6.9115, Lon 79.8691
SOUTH = 6.903
WEST = 79.860
NORTH = 6.920
EAST = 79.878

# Active public Overpass instances to try in order
OVERPASS_ENDPOINTS = [
    "https://overpass-api.de/api/interpreter",
    "https://overpass.private.coffee/api/interpreter",
    "https://lz4.overpass-api.de/api/interpreter",
    "https://z.overpass-api.de/api/interpreter"
]

query = f"""[out:json][timeout:180];
(
  node["highway"="traffic_signals"]({SOUTH},{WEST},{NORTH},{EAST});
  way["highway"]({SOUTH},{WEST},{NORTH},{EAST});
  way["building"]({SOUTH},{WEST},{NORTH},{EAST});
  relation["building"]({SOUTH},{WEST},{NORTH},{EAST});
);
out body;
>;
out skel qt;"""

def fetch_osm_data(query_str):
    data = urllib.parse.urlencode({"data": query_str}).encode("utf-8")
    
    for endpoint in OVERPASS_ENDPOINTS:
        print(f"Sending query to Overpass API endpoint: {endpoint} ...")
        req = urllib.request.Request(
            endpoint, 
            data=data, 
            headers={"User-Agent": "AlgoCityGL Map Fetcher (contact: support@algocity.org)"}
        )
        
        try:
            # Set socket timeout to 300 seconds (5 minutes)
            with urllib.request.urlopen(req, timeout=300) as response:
                print(f"Success! Data received from {endpoint}")
                return json.loads(response.read().decode("utf-8"))
        except (urllib.error.URLError, socket.timeout, Exception) as e:
            print(f"Endpoint {endpoint} failed or timed out: {e}")
            print("Trying next fallback endpoint...")
            
    raise RuntimeError("All configured Overpass API endpoints failed or timed out.")

def convert_osm_to_geojson(osm_data):
    print("Converting OSM JSON to GeoJSON...")
    elements = osm_data.get("elements", [])
    
    # First pass: index nodes by ID
    nodes = {}
    for el in elements:
        if el["type"] == "node":
            nodes[el["id"]] = (el["lon"], el["lat"])
            
    features = []
    
    # Second pass: process ways and nodes
    for el in elements:
        el_type = el["type"]
        el_id = el["id"]
        tags = el.get("tags", {})
        
        if el_type == "way":
            way_nodes = el.get("nodes", [])
            coords = [nodes[nid] for nid in way_nodes if nid in nodes]
            
            if len(coords) < 2:
                continue
                
            is_polygon = (coords[0] == coords[-1]) and ("building" in tags)
            
            # Map tag keys appropriately
            properties = {"id": f"way/{el_id}"}
            properties.update(tags)
            
            geometry = {
                "type": "Polygon" if is_polygon else "LineString",
                "coordinates": [coords] if is_polygon else coords
            }
            
            features.append({
                "type": "Feature",
                "id": f"way/{el_id}",
                "properties": properties,
                "geometry": geometry
            })
            
        elif el_type == "node" and tags.get("highway") == "traffic_signals":
            properties = {"id": f"node/{el_id}"}
            properties.update(tags)
            
            geometry = {
                "type": "Point",
                "coordinates": [el["lon"], el["lat"]]
            }
            
            features.append({
                "type": "Feature",
                "id": f"node/{el_id}",
                "properties": properties,
                "geometry": geometry
            })
            
    return {
        "type": "FeatureCollection",
        "features": features
    }

def main():
    try:
        osm_raw = fetch_osm_data(query)
        geojson = convert_osm_to_geojson(osm_raw)
        
        output_dir = Path("data/osm")
        output_dir.mkdir(parents=True, exist_ok=True)
        output_path = output_dir / "borella_raw_osm.geojson"
        
        with open(output_path, "w", encoding="utf-8") as f:
            json.dump(geojson, f, indent=2)
            
        print(f"Successfully saved {len(geojson['features'])} features to {output_path}")
        
    except Exception as e:
        print(f"Failed to fetch or convert OSM data: {e}")
        exit(1)

if __name__ == "__main__":
    main()
