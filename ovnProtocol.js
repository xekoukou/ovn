module.exports = {

    request: {
        type: "object",
        properties: {
            requestId: {
                type: "integer"
            }
            request: {
                type: "object",
                oneof: [{
                    "$ref": "#/definitions/retrieveRequest"
                    "$ref": "#/definitions/newNodeRequest"
                }]
            }

        },
        required: ["requestId", "request"]


    },

    response: {
        type: "object",
        properties: {
            requestId: {
                type: "integer"
            }
            response: {
                type: "object",
                oneof: [{
                    "$ref": "#/definitions/retrieveResponse"
                    "$ref": "#/definitions/newNodeResponse"
                }]
            }
        },
        required: ["requestId", "response"]


    },

    definitions: [

        retrieveRequest: {
            type: "object",
            properties: {
                "type": {
                    "enum": ["retrieveRequest"]
                },

                idArray: {
                    type: "array",
                    minItems: 1,
                    items: {
                        type: "integer"
                    }


                },
                required: ["type", "idArray"]
            }
        },
        newNodeRequest: {
            type: "object",
            properties: {
                "type": {
                    "enum": ["newNodeRequest"]
                },

                parentId: {
                    type: "integer",
                    minimum: 0
                },
                nodeData: {
                    type: "object",
                }


            },
            required: ["type", "parentId", "nodeData"]
        },
        retrieveResponse: {

            type: "object",
            properties: {
                "type": {
                    "enum": ["retrieveResponse"]
                },

                nodeArray: {
                    type: "array",
                    minItems: 1,
                    items: {
                        type: "object",
                        "$ref": "node.js#/node"

                    }
                }
            },
            required: ["type", "nodeArray"]
        },
        newNodeResponse: {
            type: "object",
            properties: {
                "type": {
                    "enum": ["newNodeResponse"]
                },

                id: {
                    type: "integer",
                }


            },
            required: ["type", "id"]

        }
    ]


}
