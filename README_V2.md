Watson Developer Cloud C++ SDK
============================================
**THE FOLLOWING SECTION COPIED FROM NODE SDK AND MUST BE REPLACED FOR C++ SDK**
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/460c1d01a56942dbb7dd15d9ee0da535)](https://www.codacy.com/app/gattana/node-sdk?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=watson-developer-cloud/node-sdk&amp;utm_campaign=Badge_Grade)
[![Build Status](https://secure.travis-ci.org/watson-developer-cloud/node-sdk.svg)](http://travis-ci.org/watson-developer-cloud/node-sdk)
[![Dependency Status](https://gemnasium.com/watson-developer-cloud/node-sdk.svg)](https://gemnasium.com/watson-developer-cloud/node-sdk)
[![codecov.io](https://codecov.io/github/watson-developer-cloud/node-sdk/coverage.svg?branch=master)](https://codecov.io/github/watson-developer-cloud/node-sdk?branch=master)
[![npm-version](https://img.shields.io/npm/v/watson-developer-cloud.svg)](https://www.npmjs.com/package/watson-developer-cloud)
[![npm-downloads](https://img.shields.io/npm/dm/watson-developer-cloud.svg)](https://www.npmjs.com/package/watson-developer-cloud)

The C++ SDK uses the Watson Developer Cloud services, a collection of REST APIs, and SDKs that use cognitive computing to solve complex problems.

## Table of Contents
  * [Installation](#installation)
  * [Usage](#usage)
  * [Getting the Service Credentials](#getting-the-service-credentials)
  * [Questions](#questions)
  * [Examples](#examples)
  * [IBM Watson Services](#ibm-watson-services)
    * [AlchemyLanguage](#alchemylanguage)
    * [AlchemyVision](#alchemyvision)
    * [AlchemyData News](#alchemydata-news)
    * [Authorization](#authorization)
    * [Concept Insights](#concept-insights)
    * [Conversation](#conversation)
    * [Document Conversion](#document-conversion)
    * [Language Translator](#language-translator)
    * [Natural Language Classifier](#natural-language-classifier)
    * [Personality Insights](#personality-insights)
    * [Relationship Extraction](#relationship-extraction)
    * [Retrieve and Rank](#retrieve-and-rank)
    * [Speech to Text](#speech-to-text)
    * [Text to Speech](#text-to-speech)
    * [Tone Analyzer](#tone-analyzer)
    * [Tradeoff Analytics](#tradeoff-analytics)
    * [Visual Insights](#visual-insights)
    * [Visual Recognition](#visual-recognition)
  * [Composing Services](#composing-services)
  * [Debug](#debug)
  * [Tests](#tests)
  * [Open Source @ IBM](#open-source--ibm)
  * [License](#license)
  * [Contributing](#contributing)

## Installation
**THIS SECTION REQUIRES CODE**
```
GET INSTALLATION CODE
```

## Usage

The examples below assume that you already have service credentials. If not,
you will have to create a service in [Bluemix][bluemix].

If you are running your application in Bluemix, you don't need to specify the
credentials; the library will get them for you by looking at the `VCAP_SERVICES` environment variable.

### Data collection opt-out
**DO WE NEED THIS SECTION?**
By default, [all requests are logged](http://www.ibm.com/watson/developercloud/doc/getting_started/gs-logging.shtml). This can be disabled of by setting the `X-Watson-Learning-Opt-Out` header when creating the service instance:

```
GET CODE SAMPLE
```

## Getting the Service Credentials
You will need the `username` and `password` (`api_key` for AlchemyAPI) credentials for each service. Service credentials are different from your Bluemix account username and password.

To get your service credentials, follow these steps:
 1. Log in to Bluemix at https://bluemix.net.

 1. Create an instance of the service:
     1. In the Bluemix **Catalog**, select the service you want to use.
     1. Under **Add Service**, type a unique name for the service instance in the Service name field. For example, type `my-service-name`. Leave the default values for the other options.
     1. Click **Create**.

 1. Copy your credentials:
     1. On the left side of the page, click **Service Credentials** to view your service credentials.
     1. Copy `username` and `password`(`api_key` for AlchemyAPI).

## Questions

If you are having difficulties using the APIs or have a question about the IBM
Watson Services, please ask a question on
[dW Answers](https://developer.ibm.com/answers/questions/ask/?topics=watson)
or [Stack Overflow](http://stackoverflow.com/questions/ask?tags=ibm-watson).

## Examples
**DO WE HAVE EXAMPLES, OR SHOULD THE FOLLOWING SECTION BE REMOVED?**
The [examples][examples] folder has basic and advanced examples.

## IBM Watson Services
The Watson Developer Cloud offers a variety of services for building cognitive
apps.

### AlchemyLanguage
[AlchemyLanguage][alchemy_language] offers 12 API functions as part of its text analysis service, each of which uses sophisticated natural language processing techniques to analyze your content and add high-level semantic information.

Use the [Sentiment Analysis][sentiment_analysis] endpoint to identify positive/negative sentiment within a sample text document.

```
GET CODE SAMPLE
```

### AlchemyVision
The AlchemyVision service has been replace by the [Visual Recognition](#visual-recognition) service. Existing users have until May 20, 2017 to migrate to the new service, and no new instances may be created.

### AlchemyData News
[Alchemy Data News][alchemy_data_news] indexes 250k to 300k English language news and blog articles every day with historical search available for the past 60 days.
Example: Get the volume data from the last 7 days using 12hs of time slice.

```
GET CODE SAMPLE
```

### Authorization
The Authorization service can generates auth tokens for situations where providing the service username/password is undesirable.

Tokens are valid for 1 hour and may be sent using the `X-Watson-Authorization-Token` header or the `watson-token` query param.

Note that the token is supplied URL-encoded, and will not be accepted if it is double-encoded in a querystring.

```
GET CODE SAMPLE
```

### Concept Insights

The [Concept Insights][concept_insights] has been deprecated, AlchemyLanguage's concept function can be used as a replacement for most Concept Insights use cases; therefore, we encourage existing Concept Insights service users to migrate to AlchemyLanguage.


### Conversation

Use the [Conversation][conversation] service to determine the intent of a message.

Note: you must first create a workspace via Bluemix. See [the documentation](http://www.ibm.com/watson/developercloud/doc/conversation/overview.shtml) for details.

```
GET CODE SAMPLE
```

### Document Conversion

```
GET CODE SAMPLE
```

See the [Document Conversion integration example][document_conversion_integration_example] about how to integrate the Document Conversion service with the Retrieve and Rank service.

### Language Translator

Translate text from one language to another or idenfity a language using the [Language Translator][language_translator] service.

```
GET CODE SAMPLE
```

### Natural Language Classifier

Use [Natural Language Classifier](http://www.ibm.com/watson/developercloud/doc/nl-classifier/) service to create a classifier instance by providing a set of representative strings and a set of one or more correct classes for each as training. Then use the trained classifier to classify your new question for best matching answers or to retrieve next actions for your application.

```
GET CODE SAMPLE
```

See this [example](https://github.com/watson-developer-cloud/node-sdk/blob/master/examples/natural_language_classifier.v1.js) to learn how to create a classifier.

### Personality Insights
Analyze text in english and get a personality profile by using the
[Personality Insights][personality_insights] service.

```
GET CODE SAMPLE
```

**Important:** Don't forget to update the `text` variable.


### Relationship Extraction
Relationship Extraction has been deprecated. If you want to continue using Relationship Extraction models, you can now access them with AlchemyLanguage. See the [migration guide][re_migration] for details.


### Retrieve and Rank
Use the [Retrieve and Rank][retrieve_and_rank] service to enhance search results with machine learning.

```
GET CODE SAMPLE
```

### Speech to Text
Use the [Speech to Text][speech_to_text] service to recognize the text from a .wav file.

```
GET CODE SAMPLE
```

### Text to Speech
Use the [Text to Speech][text_to_speech] service to synthesize text into a .wav file.

```
GET CODE SAMPLE
```

### Tone Analyzer
Use the [Tone Analyzer][tone_analyzer] service to analyze the
emotion, writing and social tones of a text.

```
GET CODE SAMPLE
```

### Tradeoff Analytics
Use the [Tradeoff Analytics][tradeoff_analytics] service to find the best
phone that minimizes price and weight and maximizes screen size.

```
GET CODE SAMPLE
```

### Visual Insights
The Watson [Visual Insights][visual_insights] Service will be withdrawn. The Watson Visual Insights Service tile will be removed from the Bluemix catalog on July 3, 2016, after which you cannot provision new instances of this service.

### Visual Recognition
Use the [Visual Recognition][visual_recognition] service to recognize the
following picture.

<img src="https://visual-recognition-demo.mybluemix.net/images/samples/5.jpg" width="150" />

```
GET CODE SAMPLE
```

## Composing Services

### Integration of Tone Analyzer with Conversation
Sample code for [integrating Tone Analyzer and Conversation][conversation_tone_analyzer_example] is provided in the [examples directory][examples].

## Integration Document Conversation with Retrieve and Rank
See the [Document Conversion integration example][document_conversion_integration_example] about how to integrate the Document Conversion service
with the Retrieve and Rank service.

## Unauthenticated requests
By default, the library tries to use Basic Auth and will ask for `api_key` or `username` and `password` and send an `Authorization: Basic XXXXXXX`. You can avoid this by using:

`use_unauthenticated`.

```
GET CODE SAMPLE
```

## Debug
**DO WE NEED THIS SECTION?**
This library relies on the `request` npm module writted by
[request][request_github] to call the Watson Services. To debug the apps, add
'request' to the `NODE_DEBUG` environment variable:

```sh
$ NODE_DEBUG='request' node app.js
```
where `app.js` is your Node.js file.

## Tests
**DO WE NEED THIS SECTION?**
Running all the tests:
```sh
$ npm test
```

Running a specific test:
```sh
$ mocha -g '<test name>'
```

## Open Source @ IBM
[Find more open source projects on the IBM Github Page.](http://ibm.github.io/)

## License

This library is licensed under Apache 2.0. Full license text is available in
[COPYING][license].

## Contributing
See [CONTRIBUTING](https://github.com/watson-developer-cloud/node-sdk/blob/master/.github/CONTRIBUTING.md).

[conversation]: https://www.ibm.com/watson/developercloud/conversation.html

[personality_insights]: http://www.ibm.com/watson/developercloud/doc/personality-insights/
[relationship_extraction]: http://www.ibm.com/watson/developercloud/doc/sireapi/
[retrieve_and_rank]: http://watson.stage1.mybluemix.net/watson/developercloud/retrieve-rank.html
[visual_recognition]: http://www.ibm.com/watson/developercloud/doc/visual-recognition/
[visual_insights]: http://www.ibm.com/watson/developercloud/doc/visual-insights/

[tone_analyzer]: http://www.ibm.com/watson/developercloud/tone-analyzer.html
[text_to_speech]: http://www.ibm.com/watson/developercloud/doc/text-to-speech/
[speech_to_text]: http://www.ibm.com/watson/developercloud/doc/speech-to-text/
[concept_insights]: http://www.ibm.com/watson/developercloud/doc/concept-insights/
[tradeoff_analytics]: http://www.ibm.com/watson/developercloud/doc/tradeoff-analytics/
[language_translator]: http://www.ibm.com/watson/developercloud/doc/language-translation/
[re_migration]: http://www.ibm.com/watson/developercloud/doc/alchemylanguage/migration.shtml

[alchemy_language]: http://www.alchemyapi.com/products/alchemylanguage
[sentiment_analysis]: http://www.alchemyapi.com/products/alchemylanguage/sentiment-analysis
[alchemy_vision]: http://www.alchemyapi.com/products/alchemyvision
[alchemy_data_news]: http://www.alchemyapi.com/products/alchemydata-news

[wdc]: http://www.ibm.com/watson/developercloud/
[bluemix]: https://console.ng.bluemix.net
[npm_link]: https://www.npmjs.com/package/watson-developer-cloud
[request_github]: https://github.com/request/request
[dialog_migration]: https://www.ibm.com/watson/developercloud/doc/conversation/migration.shtml

[examples]: https://github.com/watson-developer-cloud/node-sdk/tree/master/examples
[document_conversion_integration_example]: https://github.com/watson-developer-cloud/node-sdk/tree/master/examples/document_conversion_integration.v1.js
[conversation_tone_analyzer_example]: https://github.com/watson-developer-cloud/node-sdk/tree/master/examples/conversation_tone_analyzer_integration

[license]: http://www.apache.org/licenses/LICENSE-2.0
