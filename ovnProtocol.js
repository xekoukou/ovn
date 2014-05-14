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
                }, {
                    "$ref": "#/definitions/newNodeRequest"
                }, {
                    "$ref": "../clientNodejsProtocol.js#/request/definitions/newLink"
                }, {
                    "$ref": "../clientNodejsProtocol.js#/request/definitions/delLink"
                }]
            }

        },
        required: ["requestId", "request"],

        definitions: {

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
                        "enum": ["newNode"]
                    },

                    node: {
                        type: "object",
                    }


                },
                required: ["type", "node"]
            }
        }


    }


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
                }, {
                    "$ref": "#/definitions/newNodeResponse"
                }, {
                    "$ref": "#/definitions/newLinkResponse"
                }, {
                    "$ref": "#/definitions/delLinkResponse"
                }

            ]
        }
    },
    required: ["requestId", "response"],


    definitions: {

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
                        type: "object"
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

        },
        newLinkResponse: {
            type: "object",
            properties: {
                "type": {
                    "enum": ["newLinkResponse"]
                },

                id: {
                    type: "integer",
                }


            },
            required: ["type", "id"]

        },
        delLinkResponse: {
            type: "object",
            properties: {
                "type": {
                    "enum": ["delLinkResponse"]
                },

                ack: {
                    "enum": ["ok", "fail"]
                }
            },
            required: ["type", "id"]

        }


    }
}


}
